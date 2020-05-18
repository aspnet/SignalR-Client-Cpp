// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#include "stdafx.h"
#include "hub_connection_impl.h"
#include "signalrclient/hub_exception.h"
#include "trace_log_writer.h"
#include "signalrclient/signalr_exception.h"
#include "json_hub_protocol.h"
#include "message_type.h"
#include "handshake_protocol.h"
#include "signalrclient/websocket_client.h"

namespace signalr
{
    // unnamed namespace makes it invisble outside this translation unit
    namespace
    {
        static std::function<void(const signalr::value&)> create_hub_invocation_callback(const logger& logger,
            const std::function<void(const signalr::value&)>& set_result,
            const std::function<void(const std::exception_ptr e)>& set_exception);
    }

    std::shared_ptr<hub_connection_impl> hub_connection_impl::create(const std::string& url, trace_level trace_level,
        const std::shared_ptr<log_writer>& log_writer, std::shared_ptr<http_client> http_client,
        std::function<std::shared_ptr<websocket_client>(const signalr_client_config&)> websocket_factory, const bool skip_negotiation)
    {
        auto connection = std::shared_ptr<hub_connection_impl>(new hub_connection_impl(url, trace_level, log_writer, http_client, websocket_factory, skip_negotiation));

        connection->initialize();

        return connection;
    }

    hub_connection_impl::hub_connection_impl(const std::string& url, trace_level trace_level,
        const std::shared_ptr<log_writer>& log_writer, std::shared_ptr<http_client> http_client,
        std::function<std::shared_ptr<websocket_client>(const signalr_client_config&)> websocket_factory, const bool skip_negotiation)
        : m_connection(connection_impl::create(url, trace_level, log_writer,
            http_client, websocket_factory, skip_negotiation)), m_logger(log_writer, trace_level),
        m_callback_manager(signalr::value(std::map<std::string, signalr::value> { { std::string("error"), std::string("connection went out of scope before invocation result was received") } })),
        m_handshakeReceived(false), m_disconnected([]() noexcept {}), m_protocol(std::make_shared<json_hub_protocol>())
    {}

    void hub_connection_impl::initialize()
    {
        // weak_ptr prevents a circular dependency leading to memory leak and other problems
        std::weak_ptr<hub_connection_impl> weak_hub_connection = shared_from_this();

        m_connection->set_message_received([weak_hub_connection](std::string&& message)
        {
            auto connection = weak_hub_connection.lock();
            if (connection)
            {
                connection->process_message(std::move(message));
            }
        });

        m_connection->set_disconnected([weak_hub_connection]()
        {
            auto connection = weak_hub_connection.lock();
            if (connection)
            {
                // start may be waiting on the handshake response so we complete it here, this no-ops if already set
                connection->m_handshakeTask->set(std::make_exception_ptr(signalr_exception("connection closed while handshake was in progress.")));

                connection->m_callback_manager.clear(signalr::value(std::map<std::string, signalr::value> { { std::string("error"), std::string("connection was stopped before invocation result was received") } }));

                connection->m_disconnected();
            }
        });
    }

    void hub_connection_impl::on(const std::string& event_name, const std::function<void(const signalr::value &)>& handler)
    {
        if (event_name.length() == 0)
        {
            throw std::invalid_argument("event_name cannot be empty");
        }

        auto weak_connection = std::weak_ptr<hub_connection_impl>(shared_from_this());
        auto connection = weak_connection.lock();
        if (connection && connection->get_connection_state() != connection_state::disconnected)
        {
            throw signalr_exception("can't register a handler if the connection is in a disconnected state");
        }

        if (m_subscriptions.find(event_name) != m_subscriptions.end())
        {
            throw signalr_exception(
                "an action for this event has already been registered. event name: " + event_name);
        }

        m_subscriptions.insert({event_name, handler});
    }

    void hub_connection_impl::start(std::function<void(std::exception_ptr)> callback) noexcept
    {
        if (m_connection->get_connection_state() != connection_state::disconnected)
        {
            callback(std::make_exception_ptr(signalr_exception(
                "the connection can only be started if it is in the disconnected state")));
            return;
        }

        m_connection->set_client_config(m_signalr_client_config);
        m_handshakeTask = std::make_shared<completion_event>();
        m_handshakeReceived = false;
        std::weak_ptr<hub_connection_impl> weak_connection = shared_from_this();
        m_connection->start([weak_connection, callback](std::exception_ptr start_exception)
            {
                auto connection = weak_connection.lock();
                if (!connection)
                {
                    // The connection has been destructed
                    callback(std::make_exception_ptr(signalr_exception("the hub connection has been deconstructed")));
                    return;
                }

                if (start_exception)
                {
                    connection->m_connection->stop([start_exception, callback, weak_connection](std::exception_ptr)
                    {
                        try
                        {
                            auto connection = weak_connection.lock();
                            if (!connection)
                            {
                                callback(std::make_exception_ptr(signalr_exception("the hub connection has been deconstructed")));
                                return;
                            }
                        }
                        catch (...) {}

                        callback(start_exception);
                    });
                    return;
                }

                auto handshake_request = handshake::write_handshake(connection->m_protocol);

                connection->m_connection->send(handshake_request, [weak_connection, callback](std::exception_ptr exception)
                {
                    auto connection = weak_connection.lock();
                    if (!connection)
                    {
                        // The connection has been destructed
                        callback(std::make_exception_ptr(signalr_exception("the hub connection has been deconstructed")));
                        return;
                    }

                    if (exception)
                    {
                        callback(exception);
                        return;
                    }

                    try
                    {
                        connection->m_handshakeTask->get();
                        callback(nullptr);
                    }
                    catch (...)
                    {
                        auto handshake_exception = std::current_exception();
                        connection->m_connection->stop([callback, handshake_exception](std::exception_ptr)
                        {
                            callback(handshake_exception);
                        });
                    }
                });
            });
    }

    void hub_connection_impl::stop(std::function<void(std::exception_ptr)> callback) noexcept
    {
        if (get_connection_state() == connection_state::disconnected)
        {
            m_logger.log(trace_level::info, "Stop ignored because the connection is already disconnected.");
            callback(nullptr);
            return;
        }
        else
        {
            {
                std::lock_guard<std::mutex> lock(m_stop_callback_lock);
                m_stop_callbacks.push_back(callback);

                if (m_stop_callbacks.size() > 1)
                {
                    m_logger.log(trace_level::info, "Stop is already in progress, waiting for it to finish.");
                    // we already registered the callback
                    // so we can just return now as the in-progress stop will trigger the callback when it completes
                    return;
                }
            }
            std::weak_ptr<hub_connection_impl> weak_connection = shared_from_this();
            m_connection->stop([weak_connection](std::exception_ptr exception)
                {
                    auto connection = weak_connection.lock();
                    if (!connection)
                    {
                        return;
                    }

                    std::vector<std::function<void(std::exception_ptr)>> callbacks;

                    {
                        std::lock_guard<std::mutex> lock(connection->m_stop_callback_lock);
                        // copy the callbacks out and clear the list inside the lock
                        // then run the callbacks outside of the lock
                        callbacks = connection->m_stop_callbacks;
                        connection->m_stop_callbacks.clear();
                    }

                    for (auto& callback : callbacks)
                    {
                        callback(exception);
                    }
                });
        }
    }

    void hub_connection_impl::process_message(std::string&& response)
    {
        try
        {
            if (!m_handshakeReceived)
            {
                signalr::value handshake;
                std::tie(response, handshake) = handshake::parse_handshake(response);

                auto& obj = handshake.as_map();
                auto found = obj.find("error");
                if (found != obj.end())
                {
                    auto& error = found->second.as_string();
                    m_logger.log(trace_level::errors, std::string("handshake error: ")
                        .append(error));
                    m_handshakeTask->set(std::make_exception_ptr(signalr_exception(std::string("Received an error during handshake: ").append(error))));
                    return;
                }
                else
                {
                    found = obj.find("type");
                    if (found != obj.end())
                    {
                        m_handshakeTask->set(std::make_exception_ptr(signalr_exception(std::string("Received unexpected message while waiting for the handshake response."))));
                    }

                    m_handshakeReceived = true;
                    m_handshakeTask->set();

                    if (response.size() == 0)
                    {
                        return;
                    }
                }
            }

            auto messages = m_protocol->parse_messages(response);

            for (const auto& val : messages)
            {
                if (!val.is_map())
                {
                    m_logger.log(trace_level::info, std::string("unexpected response received from the server: ")
                        .append(response));

                    return;
                }

                const auto &obj = val.as_map();

                switch ((int)obj.at("type").as_double())
                {
                case message_type::invocation:
                {
                    auto method = obj.at("target").as_string();
                    auto event = m_subscriptions.find(method);
                    if (event != m_subscriptions.end())
                    {
                        const auto& args = obj.at("arguments");
                        event->second(args);
                    }
                    else
                    {
                        m_logger.log(trace_level::info, "handler not found");
                    }
                    break;
                }
                case message_type::stream_invocation:
                    // Sent to server only, should not be received by client
                    throw std::runtime_error("Received unexpected message type 'StreamInvocation'");
                case message_type::stream_item:
                    // TODO
                    break;
                case message_type::completion:
                {
                    auto error = obj.find("error");
                    auto result = obj.find("result");
                    if (error != obj.end() && result != obj.end())
                    {
                        // TODO: error
                    }
                    invoke_callback(obj);
                    break;
                }
                case message_type::cancel_invocation:
                    // Sent to server only, should not be received by client
                    throw std::runtime_error("Received unexpected message type 'CancelInvocation'.");
                case message_type::ping:
                    // TODO
                    break;
                case message_type::close:
                    // TODO
                    break;
                }
            }
        }
        catch (const std::exception &e)
        {
            m_logger.log(trace_level::errors, std::string("error occured when parsing response: ")
                .append(e.what())
                .append(". response: ")
                .append(response));

            // TODO: Consider passing "reason" exception to stop
            m_connection->stop([](std::exception_ptr) {});
        }
    }

    bool hub_connection_impl::invoke_callback(const signalr::value& message)
    {
        if (!message.is_map())
        {
            throw signalr_exception("expected object");
        }

        auto& invocationId = message.as_map().at("invocationId");
        if (!invocationId.is_string())
        {
            throw signalr_exception("invocationId is not a string");
        }

        auto& id = invocationId.as_string();
        if (!m_callback_manager.invoke_callback(id, message, true))
        {
            m_logger.log(trace_level::info, std::string("no callback found for id: ").append(id));
            return false;
        }

        return true;
    }

    void hub_connection_impl::invoke(const std::string& method_name, const signalr::value& arguments, std::function<void(const signalr::value&, std::exception_ptr)> callback) noexcept
    {
        if (!arguments.is_array())
        {
            callback(signalr::value(), std::make_exception_ptr(signalr_exception("arguments should be an array")));
            return;
        }

        const auto& callback_id = m_callback_manager.register_callback(
            create_hub_invocation_callback(m_logger, [callback](const signalr::value& result) { callback(result, nullptr); },
                [callback](const std::exception_ptr e) { callback(signalr::value(), e); }));

        invoke_hub_method(method_name, arguments, callback_id, nullptr,
            [callback](const std::exception_ptr e){ callback(signalr::value(), e); });
    }

    void hub_connection_impl::send(const std::string& method_name, const signalr::value& arguments, std::function<void(std::exception_ptr)> callback) noexcept
    {
        if (!arguments.is_array())
        {
            callback(std::make_exception_ptr(signalr_exception("arguments should be an array")));
            return;
        }

        invoke_hub_method(method_name, arguments, "",
            [callback]() { callback(nullptr); },
            [callback](const std::exception_ptr e){ callback(e); });
    }

    void hub_connection_impl::invoke_hub_method(const std::string& method_name, const signalr::value& arguments,
        const std::string& callback_id, std::function<void()> set_completion, std::function<void(const std::exception_ptr)> set_exception) noexcept
    {
        auto map = std::map<std::string, signalr::value>
        {
            { "type", signalr::value((double)message_type::invocation) },
            { "target", signalr::value(method_name) },
            { "arguments", arguments }
        };

        if (!callback_id.empty())
        {
            map["invocationId"] = signalr::value(callback_id);
        }

        auto message = m_protocol->write_message(signalr::value(std::move(map)));

        // weak_ptr prevents a circular dependency leading to memory leak and other problems
        auto weak_hub_connection = std::weak_ptr<hub_connection_impl>(shared_from_this());

        m_connection->send(message, [set_completion, set_exception, weak_hub_connection, callback_id](std::exception_ptr exception)
        {
            if (exception)
            {
                auto hub_connection = weak_hub_connection.lock();
                if (hub_connection)
                {
                    hub_connection->m_callback_manager.remove_callback(callback_id);
                }
                set_exception(exception);
            }
            else
            {
                if (callback_id.empty())
                {
                    // complete nonBlocking call
                    set_completion();
                }
            }
        });
    }

    connection_state hub_connection_impl::get_connection_state() const noexcept
    {
        return m_connection->get_connection_state();
    }

    std::string hub_connection_impl::get_connection_id() const
    {
        return m_connection->get_connection_id();
    }

    void hub_connection_impl::set_client_config(const signalr_client_config& config)
    {
        m_signalr_client_config = config;
        m_connection->set_client_config(config);
    }

    void hub_connection_impl::set_disconnected(const std::function<void()>& disconnected)
    {
        m_disconnected = disconnected;
    }

    // unnamed namespace makes it invisble outside this translation unit
    namespace
    {
        static std::function<void(const signalr::value&)> create_hub_invocation_callback(const logger& logger,
            const std::function<void(const signalr::value&)>& set_result,
            const std::function<void(const std::exception_ptr)>& set_exception)
        {
            return [logger, set_result, set_exception](const signalr::value& message)
            {
                assert(message.is_map());
                const auto& map = message.as_map();
                auto found = map.find("result");
                if (found != map.end())
                {
                    set_result(found->second);
                }
                else if ((found = map.find("error")) != map.end())
                {
                    assert(found->second.is_string());
                    set_exception(
                        std::make_exception_ptr(
                            hub_exception(found->second.as_string())));
                }
                else
                {
                    set_result(signalr::value());
                }
            };
        }
    }
}
