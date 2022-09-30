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
#include "signalr_default_scheduler.h"

namespace signalr
{
    // unnamed namespace makes it invisble outside this translation unit
    namespace
    {
        static std::function<void(const char*, const signalr::value&)> create_hub_invocation_callback(const logger& logger,
            const std::function<void(const signalr::value&)>& set_result,
            const std::function<void(const std::exception_ptr e)>& set_exception);
    }

    std::shared_ptr<hub_connection_impl> hub_connection_impl::create(const std::string& url, std::unique_ptr<hub_protocol>&& hub_protocol,
        trace_level trace_level, const std::shared_ptr<log_writer>& log_writer, std::function<std::shared_ptr<http_client>(const signalr_client_config&)> http_client_factory,
        std::function<std::shared_ptr<websocket_client>(const signalr_client_config&)> websocket_factory, const bool skip_negotiation)
    {
        auto connection = std::shared_ptr<hub_connection_impl>(new hub_connection_impl(url, std::move(hub_protocol),
            trace_level, log_writer, http_client_factory, websocket_factory, skip_negotiation));

        connection->initialize();

        return connection;
    }

    hub_connection_impl::hub_connection_impl(const std::string& url, std::unique_ptr<hub_protocol>&& hub_protocol, trace_level trace_level,
        const std::shared_ptr<log_writer>& log_writer, std::function<std::shared_ptr<http_client>(const signalr_client_config&)> http_client_factory,
        std::function<std::shared_ptr<websocket_client>(const signalr_client_config&)> websocket_factory, const bool skip_negotiation)
        : m_connection(connection_impl::create(url, trace_level, log_writer, http_client_factory, websocket_factory, skip_negotiation))
            , m_logger(log_writer, trace_level),
        m_callback_manager("connection went out of scope before invocation result was received"),
        m_handshakeReceived(false), m_disconnected([](std::exception_ptr) noexcept {}), m_protocol(std::move(hub_protocol))
    {
        hub_message ping_msg(signalr::message_type::ping);
        m_cached_ping = m_protocol->write_message(&ping_msg);
    }

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

        m_connection->set_disconnected([weak_hub_connection](std::exception_ptr exception)
        {
            auto connection = weak_hub_connection.lock();
            if (connection)
            {
                // start may be waiting on the handshake response so we complete it here, this no-ops if already set
                connection->m_handshakeTask->set(std::make_exception_ptr(signalr_exception("connection closed while handshake was in progress.")));
                try
                {
                    connection->m_disconnect_cts->cancel();
                }
                catch (const std::exception& ex)
                {
                    if (connection->m_logger.is_enabled(trace_level::warning))
                    {
                        connection->m_logger.log(trace_level::warning, std::string("disconnect event threw an exception during connection closure: ")
                            .append(ex.what()));
                    }
                }

                connection->m_callback_manager.clear("connection was stopped before invocation result was received");

                connection->m_disconnected(exception);
            }
        });
    }

    void hub_connection_impl::on(const std::string& event_name, const std::function<void(const std::vector<signalr::value>&)>& handler)
    {
        if (event_name.length() == 0)
        {
            throw std::invalid_argument("event_name cannot be empty");
        }

        auto weak_connection = std::weak_ptr<hub_connection_impl>(shared_from_this());
        auto connection = weak_connection.lock();
        if (connection && connection->get_connection_state() != connection_state::disconnected)
        {
            throw signalr_exception("can't register a handler if the connection is not in a disconnected state");
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
        m_disconnect_cts = std::make_shared<cancellation_token_source>();
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
                    assert(connection->get_connection_state() == connection_state::disconnected);
                    // connection didn't start, don't call stop
                    callback(start_exception);
                    return;
                }

                std::shared_ptr<std::mutex> handshake_request_lock = std::make_shared<std::mutex>();
                std::shared_ptr<bool> handshake_request_done = std::make_shared<bool>();

                auto handle_handshake = [weak_connection, handshake_request_done, handshake_request_lock, callback](std::exception_ptr exception, bool fromSend)
                {
                    assert(fromSend ? *handshake_request_done : true);

                    auto connection = weak_connection.lock();
                    if (!connection)
                    {
                        // The connection has been destructed
                        callback(std::make_exception_ptr(signalr_exception("the hub connection has been deconstructed")));
                        return;
                    }

                    {
                        std::lock_guard<std::mutex> lock(*handshake_request_lock);
                        // connection.send will be waiting on the handshake task which has been set by the caller already
                        if (!fromSend && *handshake_request_done == true)
                        {
                            return;
                        }
                        *handshake_request_done = true;
                    }

                    try
                    {
                        if (exception == nullptr)
                        {
                            connection->m_handshakeTask->get();
                            callback(nullptr);
                        }
                    }
                    catch (...)
                    {
                        exception = std::current_exception();
                    }

                    if (exception != nullptr)
                    {
                        connection->m_connection->stop([callback, exception](std::exception_ptr)
                            {
                                callback(exception);
                            }, exception);
                    }
                    else
                    {
                        connection->start_keepalive();
                    }
                };

                auto handshake_request = handshake::write_handshake(connection->m_protocol);
                auto handshake_task = connection->m_handshakeTask;
                auto handshake_timeout = connection->m_signalr_client_config.get_handshake_timeout();

                connection->m_disconnect_cts->register_callback([handle_handshake, handshake_request_lock, handshake_request_done]()
                    {
                        {
                            std::lock_guard<std::mutex> lock(*handshake_request_lock);
                            // no op after connection.send returned, m_handshakeTask should be set before m_disconnect_cts is canceled
                            if (*handshake_request_done == true)
                            {
                                return;
                            }
                        }

                        // if the request isn't completed then no one is waiting on the handshake task
                        // so we need to run the callback here instead of relying on connection.send completing
                        // handshake_request_done is set in handle_handshake, don't set it here
                        handle_handshake(nullptr, false);
                    });

                timer(connection->m_signalr_client_config.get_scheduler(),
                    [handle_handshake, handshake_task, handshake_timeout, handshake_request_lock](std::chrono::milliseconds duration)
                    {
                        {
                            std::lock_guard<std::mutex> lock(*handshake_request_lock);

                            // if the task is set then connection.send is either already waiting on the handshake or has completed,
                            // or stop has been called and will be handling the callback
                            if (handshake_task->is_set())
                            {
                                return true;
                            }

                            if (duration < handshake_timeout)
                            {
                                return false;
                            }
                        }

                        auto exception = std::make_exception_ptr(signalr_exception("timed out waiting for the server to respond to the handshake message."));
                        // unblocks connection.send if it's waiting on the task
                        handshake_task->set(exception);

                        handle_handshake(exception, false);
                        return true;
                    });

                connection->m_connection->send(handshake_request, connection->m_protocol->transfer_format(),
                    [handle_handshake, handshake_request_done, handshake_request_lock](std::exception_ptr exception)
                {
                    {
                        std::lock_guard<std::mutex> lock(*handshake_request_lock);
                        if (*handshake_request_done == true)
                        {
                            // callback ran from timer or cancellation token, nothing to do here
                            return;
                        }

                        // indicates that the handshake timer doesn't need to call the callback, it just needs to set the timeout exception
                        // handle_handshake will be waiting on the handshake completion (error or success) to call the callback
                        *handshake_request_done = true;
                    }

                    handle_handshake(exception, true);
                });
            });
    }

    void hub_connection_impl::stop(std::function<void(std::exception_ptr)> callback, bool is_dtor) noexcept
    {
        if (get_connection_state() == connection_state::disconnected)
        {
            // don't log if already disconnected and stop called from dtor, it's just noise
            if (!is_dtor)
            {
                m_logger.log(trace_level::debug, "stop ignored because the connection is already disconnected.");
            }
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

                    assert(connection->get_connection_state() == connection_state::disconnected);

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
                }, nullptr);
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
                    if (m_logger.is_enabled(trace_level::error))
                    {
                        m_logger.log(trace_level::error, std::string("handshake error: ")
                            .append(error));
                    }
                    m_handshakeTask->set(std::make_exception_ptr(signalr_exception(std::string("Received an error during handshake: ").append(error))));
                    return;
                }
                else
                {
                    found = obj.find("type");
                    if (found != obj.end())
                    {
                        m_handshakeTask->set(std::make_exception_ptr(signalr_exception(std::string("Received unexpected message while waiting for the handshake response."))));
                        return;
                    }

                    m_handshakeReceived = true;
                    m_handshakeTask->set();

                    if (response.size() == 0)
                    {
                        return;
                    }
                }
            }

            reset_server_timeout();
            auto messages = m_protocol->parse_messages(response);

            for (const auto& val : messages)
            {
                // Protocol received an unknown message type and gave us a null object, close the connection like we do in other client implementations
                if (val == nullptr)
                {
                    throw std::runtime_error("null message received");
                }

                switch (val->message_type)
                {
                case message_type::invocation:
                {
                    auto invocation = static_cast<invocation_message*>(val.get());
                    auto event = m_subscriptions.find(invocation->target);
                    if (event != m_subscriptions.end())
                    {
                        const auto& args = invocation->arguments;
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
                    auto completion = static_cast<completion_message*>(val.get());
                    invoke_callback(completion);
                    break;
                }
                case message_type::cancel_invocation:
                    // Sent to server only, should not be received by client
                    throw std::runtime_error("Received unexpected message type 'CancelInvocation'.");
                case message_type::ping:
                    if (m_logger.is_enabled(trace_level::debug))
                    {
                        m_logger.log(trace_level::debug, "ping message received.");
                    }
                    break;
                case message_type::close:
                    // TODO
                    break;
                default:
                    throw std::runtime_error("unknown message type '" + std::to_string(static_cast<int>(val->message_type)) + "' received");
                    break;
                }
            }
        }
        catch (const std::exception &e)
        {
            if (m_logger.is_enabled(trace_level::error))
            {
                m_logger.log(trace_level::error, std::string("error occurred when parsing response: ")
                    .append(e.what())
                    .append(". response: ")
                    .append(response));
            }

            // TODO: Consider passing "reason" exception to stop
            m_connection->stop([](std::exception_ptr) {}, std::current_exception());
        }
    }

    bool hub_connection_impl::invoke_callback(completion_message* completion)
    {
        const char* error = nullptr;
        if (!completion->error.empty())
        {
            error = completion->error.data();
        }

        // TODO: consider transferring ownership of 'result' so that if we run user callbacks on a different thread we don't need to
        // worry about object lifetime
        if (!m_callback_manager.invoke_callback(completion->invocation_id, error, completion->result, true))
        {
            if (m_logger.is_enabled(trace_level::info))
            {
                m_logger.log(trace_level::info, std::string("no callback found for id: ").append(completion->invocation_id));
            }
        }

        return true;
    }

    void hub_connection_impl::invoke(const std::string& method_name, const std::vector<signalr::value>& arguments, std::function<void(const signalr::value&, std::exception_ptr)> callback) noexcept
    {
        const auto& callback_id = m_callback_manager.register_callback(
            create_hub_invocation_callback(m_logger, [callback](const signalr::value& result) { callback(result, nullptr); },
                [callback](const std::exception_ptr e) { callback(signalr::value(), e); }));

        invoke_hub_method(method_name, arguments, callback_id, nullptr,
            [callback](const std::exception_ptr e){ callback(signalr::value(), e); });
    }

    void hub_connection_impl::send(const std::string& method_name, const std::vector<signalr::value>& arguments, std::function<void(std::exception_ptr)> callback) noexcept
    {
        invoke_hub_method(method_name, arguments, "",
            [callback]() { callback(nullptr); },
            [callback](const std::exception_ptr e){ callback(e); });
    }

    void hub_connection_impl::invoke_hub_method(const std::string& method_name, const std::vector<signalr::value>& arguments,
        const std::string& callback_id, std::function<void()> set_completion, std::function<void(const std::exception_ptr)> set_exception) noexcept
    {
        try
        {
            invocation_message invocation(callback_id, method_name, arguments);
            auto message = m_protocol->write_message(&invocation);

            // weak_ptr prevents a circular dependency leading to memory leak and other problems
            auto weak_hub_connection = std::weak_ptr<hub_connection_impl>(shared_from_this());

            m_connection->send(message, m_protocol->transfer_format(), [set_completion, set_exception, weak_hub_connection, callback_id](std::exception_ptr exception)
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

            reset_send_ping();
        }
        catch (const std::exception& e)
        {
            m_callback_manager.remove_callback(callback_id);
            if (m_logger.is_enabled(trace_level::warning))
            {
                m_logger.log(trace_level::warning, std::string("failed to send invocation: ").append(e.what()));
            }
            set_exception(std::current_exception());
        }
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

    void hub_connection_impl::set_disconnected(const std::function<void(std::exception_ptr)>& disconnected)
    {
        m_disconnected = disconnected;
    }

    void hub_connection_impl::reset_send_ping()
    {
        auto timeMs = (std::chrono::steady_clock::now() + m_signalr_client_config.get_keepalive_interval()).time_since_epoch();
        m_nextActivationSendPing.store(std::chrono::duration_cast<std::chrono::milliseconds>(timeMs).count());
    }

    void hub_connection_impl::reset_server_timeout()
    {
        auto timeMs = (std::chrono::steady_clock::now() + m_signalr_client_config.get_server_timeout()).time_since_epoch();
        m_nextActivationServerTimeout.store(std::chrono::duration_cast<std::chrono::milliseconds>(timeMs).count());
    }

    void hub_connection_impl::start_keepalive()
    {
        if (m_logger.is_enabled(trace_level::debug))
        {
            m_logger.log(trace_level::debug, "starting keep alive timer.");
        }

        auto send_ping = [](std::shared_ptr<hub_connection_impl> connection)
        {
            if (!connection)
            {
                return;
            }

            if (connection->get_connection_state() != connection_state::connected)
            {
                return;
            }

            try
            {
                std::weak_ptr<hub_connection_impl> weak_connection = connection;
                connection->m_connection->send(
                    connection->m_cached_ping,
                    connection->m_protocol->transfer_format(), [weak_connection](std::exception_ptr exception)
                    {
                        auto connection = weak_connection.lock();
                        if (connection)
                        {
                            if (exception)
                            {
                                if (connection->m_logger.is_enabled(trace_level::warning))
                                {
                                    connection->m_logger.log(trace_level::warning, "failed to send ping!");
                                }
                            }
                            else
                            {
                                connection->reset_send_ping();
                            }
                        }
                    });
            }
            catch (const std::exception& e)
            {
                if (connection->m_logger.is_enabled(trace_level::warning))
                {
                    connection->m_logger.log(trace_level::warning, std::string("failed to send ping: ").append(e.what()));
                }
            }
        };

        send_ping(shared_from_this());
        reset_server_timeout();

        std::weak_ptr<hub_connection_impl> weak_connection = shared_from_this();
        timer(m_signalr_client_config.get_scheduler(),
            [send_ping, weak_connection](std::chrono::milliseconds)
            {
                auto connection = weak_connection.lock();

                if (!connection)
                {
                    return true;
                }

                if (connection->get_connection_state() != connection_state::connected)
                {
                    return true;
                }

                auto timeNowmSeconds =
                    std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();

                if (timeNowmSeconds > connection->m_nextActivationServerTimeout.load())
                {
                    if (connection->get_connection_state() == connection_state::connected)
                    {
                        auto error_msg = std::string("server timeout (")
                            .append(std::to_string(connection->m_signalr_client_config.get_server_timeout().count()))
                            .append(" ms) elapsed without receiving a message from the server.");
                        if (connection->m_logger.is_enabled(trace_level::warning))
                        {
                            connection->m_logger.log(trace_level::warning, error_msg);
                        }

                        connection->m_connection->stop([](std::exception_ptr)
                            {
                            }, std::make_exception_ptr(signalr_exception(error_msg)));
                    }
                }

                if (timeNowmSeconds > connection->m_nextActivationSendPing.load())
                {
                    if (connection->m_logger.is_enabled(trace_level::debug))
                    {
                        connection->m_logger.log(trace_level::debug, "sending ping to server.");
                    }
                    send_ping(connection);
                }

                return false;
            });
    }

    // unnamed namespace makes it invisble outside this translation unit
    namespace
    {
        static std::function<void(const char* error, const signalr::value&)> create_hub_invocation_callback(const logger& logger,
            const std::function<void(const signalr::value&)>& set_result,
            const std::function<void(const std::exception_ptr)>& set_exception)
        {
            return [logger, set_result, set_exception](const char* error, const signalr::value& message)
            {
                if (error != nullptr)
                {
                    set_exception(
                        std::make_exception_ptr(
                            hub_exception(error)));
                }
                else
                {
                    set_result(message);
                }
            };
        }
    }
}
