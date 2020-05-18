// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#include "stdafx.h"
#include <thread>
#include <algorithm>
#include "constants.h"
#include "connection_impl.h"
#include "negotiate.h"
#include "url_builder.h"
#include "trace_log_writer.h"
#include "signalrclient/signalr_exception.h"
#include "default_http_client.h"
#include "case_insensitive_comparison_utils.h"
#include "completion_event.h"
#include <assert.h>
#include "signalrclient/websocket_client.h"
#include "default_websocket_client.h"

namespace signalr
{
    std::shared_ptr<connection_impl> connection_impl::create(const std::string& url, trace_level trace_level, const std::shared_ptr<log_writer>& log_writer)
    {
        return connection_impl::create(url, trace_level, log_writer, nullptr, nullptr, false);
    }

    std::shared_ptr<connection_impl> connection_impl::create(const std::string& url, trace_level trace_level, const std::shared_ptr<log_writer>& log_writer,
        std::shared_ptr<http_client> http_client, std::function<std::shared_ptr<websocket_client>(const signalr_client_config&)> websocket_factory, const bool skip_negotiation)
    {
        return std::shared_ptr<connection_impl>(new connection_impl(url, trace_level,
            log_writer ? log_writer : std::make_shared<trace_log_writer>(), http_client, websocket_factory, skip_negotiation));
    }

    connection_impl::connection_impl(const std::string& url, trace_level trace_level, const std::shared_ptr<log_writer>& log_writer,
        std::unique_ptr<http_client> http_client, std::unique_ptr<transport_factory> transport_factory, const bool skip_negotiation)
        : m_base_url(url), m_connection_state(connection_state::disconnected), m_logger(log_writer, trace_level), m_transport(nullptr),
        m_transport_factory(std::move(transport_factory)), m_skip_negotiation(skip_negotiation), m_message_received([](const std::string&) noexcept {}), m_disconnected([]() noexcept {})
    {
        if (http_client != nullptr)
        {
            m_http_client = std::move(http_client);
        }
        else
        {
#ifdef USE_CPPRESTSDK
            m_http_client = std::unique_ptr<class http_client>(new default_http_client());
#endif
        }
    }

    connection_impl::connection_impl(const std::string& url, trace_level trace_level, const std::shared_ptr<log_writer>& log_writer,
        std::shared_ptr<http_client> http_client, std::function<std::shared_ptr<websocket_client>(const signalr_client_config&)> websocket_factory, const bool skip_negotiation)
        : m_base_url(url), m_connection_state(connection_state::disconnected), m_logger(log_writer, trace_level), m_transport(nullptr), m_skip_negotiation(skip_negotiation),
        m_message_received([](const std::string&) noexcept {}), m_disconnected([]() noexcept {})
    {
        if (http_client != nullptr)
        {
            m_http_client = std::move(http_client);
        }
        else
        {
#ifdef USE_CPPRESTSDK
            m_http_client = std::unique_ptr<class http_client>(new default_http_client());
#endif
        }

        if (websocket_factory == nullptr)
        {
#ifdef USE_CPPRESTSDK
            websocket_factory = [](const signalr_client_config& signalr_client_config) { return std::make_shared<default_websocket_client>(signalr_client_config); };
#endif
        }

        m_transport_factory = std::unique_ptr<transport_factory>(new transport_factory(m_http_client, websocket_factory));
    }

    connection_impl::~connection_impl()
    {
        try
        {
            // Signaling the event is safe here. We are in the dtor so noone is using this instance. There might be some
            // outstanding threads that hold on to the connection via a weak pointer but they won't be able to acquire
            // the instance since it is being destroyed. Note that the event may actually be in non-signaled state here.
            m_start_completed_event.cancel();
            completion_event completion;
            auto logger = m_logger;
            shutdown([completion, logger](std::exception_ptr exception) mutable
                {
                    if (exception != nullptr)
                    {
                        // TODO: Log?
                        try
                        {
                            std::rethrow_exception(exception);
                        }
                        catch (const std::exception& e)
                        {
                            logger.log(
                                trace_level::errors,
                                std::string("shutdown threw an exception: ")
                                .append(e.what()));
                        }
                        catch (...)
                        {
                            logger.log(
                                trace_level::errors,
                                std::string("shutdown threw an unknown exception."));
                        }
                    }

                    // make sure this is last as it will unblock the destructor
                    completion.set();
                });

            completion.get();
        }
        catch (...) // must not throw from destructors
        { }

        m_transport = nullptr;
        change_state(connection_state::disconnected);
    }

    void connection_impl::start(std::function<void(std::exception_ptr)> callback) noexcept
    {
        {
            std::lock_guard<std::mutex> lock(m_stop_lock);
            if (!change_state(connection_state::disconnected, connection_state::connecting))
            {
                callback(std::make_exception_ptr(signalr_exception("cannot start a connection that is not in the disconnected state")));
                return;
            }

            // there should not be any active transport at this point
            assert(!m_transport);

            m_disconnect_cts = std::make_shared<cancellation_token>();
            m_start_completed_event.reset();
            m_connection_id = "";
        }

        start_negotiate(m_base_url, 0, callback);
    }

    void connection_impl::start_negotiate(const std::string& url, int redirect_count, std::function<void(std::exception_ptr)> callback)
    {
        if (redirect_count >= MAX_NEGOTIATE_REDIRECTS)
        {
            change_state(connection_state::disconnected);
            callback(std::make_exception_ptr(signalr_exception("Negotiate redirection limit exceeded.")));
            return;
        }

        std::weak_ptr<connection_impl> weak_connection = shared_from_this();
        const auto& token = m_disconnect_cts;

		const auto started = [weak_connection, callback, token](std::shared_ptr<transport> transport, std::exception_ptr exception)
        {
            auto connection = weak_connection.lock();
            if (!connection)
            {
                callback(std::make_exception_ptr(signalr_exception("connection no longer exists")));
                return;
            }

            try
            {
                if (exception != nullptr)
                {
                    std::rethrow_exception(exception);
                }
                token->throw_if_cancellation_requested();
            }
            catch (const std::exception& e)
            {
                if (token->is_canceled())
                {
                    connection->m_logger.log(trace_level::info,
                        "starting the connection has been canceled.");
                }
                else
                {
                    connection->m_logger.log(trace_level::errors,
                        std::string("connection could not be started due to: ")
                        .append(e.what()));
                }

                connection->m_transport = nullptr;
                connection->change_state(connection_state::disconnected);
                connection->m_start_completed_event.cancel();
                callback(std::current_exception());
                return;
            }

            connection->m_transport = transport;

            if (!connection->change_state(connection_state::connecting, connection_state::connected))
            {
                connection->m_logger.log(trace_level::errors,
                    std::string("internal error - transition from an unexpected state. expected state: connecting, actual state: ")
                    .append(translate_connection_state(connection->get_connection_state())));

                assert(false);
            }

            connection->m_start_completed_event.cancel();
            callback(nullptr);
        };

        if (m_skip_negotiation)
            return start_transport(url, started);

        negotiate::negotiate(*m_http_client, url, m_signalr_client_config,
            [callback, weak_connection, redirect_count, token, url, started](negotiation_response&& response, std::exception_ptr exception)
            {
                auto connection = weak_connection.lock();
                if (!connection)
                {
                    callback(std::make_exception_ptr(signalr_exception("connection no longer exists")));
                    return;
                }

                if (exception != nullptr)
                {
                    try
                    {
                        std::rethrow_exception(exception);
                    }
                    catch (const std::exception& e)
                    {
                        connection->m_logger.log(trace_level::errors,
                            std::string("connection could not be started due to: ")
                            .append(e.what()));
                    }
                    connection->change_state(connection_state::disconnected);
                    callback(exception);
                    return;
                }

                if (!response.error.empty())
                {
                    connection->change_state(connection_state::disconnected);
                    callback(std::make_exception_ptr(signalr_exception(response.error)));
                    return;
                }

                if (!response.url.empty())
                {
                    if (!response.accessToken.empty())
                    {
                        auto& headers = connection->m_signalr_client_config.get_http_headers();
                        headers["Authorization"] = "Bearer " + response.accessToken;
                    }
                    connection->start_negotiate(response.url, redirect_count + 1, callback);
                    return;
                }

                connection->m_connection_id = std::move(response.connectionId);
                connection->m_connection_token = std::move(response.connectionToken);

                // TODO: fallback logic

                bool foundWebsockets = false;
                for (auto& availableTransport : response.availableTransports)
                {
                    case_insensitive_equals comparer;
                    if (comparer(availableTransport.transport, "WebSockets"))
                    {
                        foundWebsockets = true;
                        break;
                    }
                }

                if (!foundWebsockets)
                {
                    connection->change_state(connection_state::disconnected);
                    callback(std::make_exception_ptr(signalr_exception("The server does not support WebSockets which is currently the only transport supported by this client.")));
                    return;
                }

                // TODO: use transfer format

                if (token->is_canceled())
                {
                    connection->change_state(connection_state::disconnected);
                    callback(std::make_exception_ptr(canceled_exception()));
                    return;
                }

                connection->start_transport(url, started);
            });
    }

    void connection_impl::start_transport(const std::string& url, std::function<void(std::shared_ptr<transport>, std::exception_ptr)> callback)
    {
        auto connection = shared_from_this();

        std::shared_ptr<bool> connect_request_done = std::make_shared<bool>();
        std::shared_ptr<std::mutex> connect_request_lock = std::make_shared<std::mutex>();

        auto weak_connection = std::weak_ptr<connection_impl>(connection);
        const auto& disconnect_cts = m_disconnect_cts;
        const auto& logger = m_logger;

        auto transport = connection->m_transport_factory->create_transport(
            transport_type::websockets, connection->m_logger, connection->m_signalr_client_config);

        transport->on_close([weak_connection](std::exception_ptr exception)
            {
                auto connection = weak_connection.lock();
                if (!connection)
                {
                    return;
                }

                // close callback will only be called if start on the transport has already returned
                // wait for the event in order to avoid a race where the state hasn't changed from connecting
                // yet and the transport errors out
                connection->m_start_completed_event.wait();
                connection->stop_connection(exception);
            });

        transport->on_receive([disconnect_cts, connect_request_done, connect_request_lock, logger, weak_connection, callback](std::string&& message, std::exception_ptr exception)
            {
                if (exception == nullptr)
                {
                    if (disconnect_cts->is_canceled())
                    {
                        logger.log(trace_level::info,
                            std::string{ "ignoring stray message received after connection was restarted. message: " }
                        .append(message));
                        return;
                    }

                    auto connection = weak_connection.lock();
                    if (connection)
                    {
                        connection->process_response(std::move(message));
                    }
                }
                else
                {
                    try
                    {
                        // Rethrowing the exception so we can log it
                        std::rethrow_exception(exception);
                    }
                    catch (const std::exception & e)
                    {
                        // When a connection is stopped we don't wait for its transport to stop. As a result if the same connection
                        // is immediately re-started the old transport can still invoke this callback. To prevent this we capture
                        // the disconnect_cts by value which allows distinguishing if the error is for the running connection
                        // or for the one that was already stopped. If this is the latter we just ignore it.
                        if (disconnect_cts->is_canceled())
                        {
                            logger.log(trace_level::info,
                                std::string{ "ignoring stray error received after connection was restarted. error: " }
                            .append(e.what()));

                            return;
                        }

                        bool run_callback = false;
                        {
                            std::lock_guard<std::mutex> lock(*connect_request_lock);
                            // no op after connection started successfully
                            if (*connect_request_done == false)
                            {
                                *connect_request_done = true;
                                run_callback = true;
                            }
                        }

                        if (run_callback)
                        {
                            callback({}, exception);
                        }
                    }
                }
            });

        std::thread([disconnect_cts, connect_request_done, connect_request_lock, callback, weak_connection]()
        {
            disconnect_cts->wait(5000);

            bool run_callback = false;
            {
                std::lock_guard<std::mutex> lock(*connect_request_lock);
                // no op after connection started successfully
                if (*connect_request_done == false)
                {
                    *connect_request_done = true;
                    run_callback = true;
                }
            }

            // if the disconnect_cts is canceled it means that the connection has been stopped or went out of scope in
            // which case we should not throw due to timeout.
            if (disconnect_cts->is_canceled())
            {
                if (run_callback)
                {
                    // The callback checks the disconnect_cts token and will handle it appropriately
                    callback({}, nullptr);
                }
            }
            else
            {
                if (run_callback)
                {
                    callback({}, std::make_exception_ptr(signalr_exception("transport timed out when trying to connect")));
                }
            }
        }).detach();

        connection->send_connect_request(transport, url, [callback, connect_request_done, connect_request_lock, transport](std::exception_ptr exception)
            {
                bool run_callback = false;
                {
                    std::lock_guard<std::mutex> lock(*connect_request_lock);
                    // no op after connection started successfully
                    if (*connect_request_done == false)
                    {
                        *connect_request_done = true;
                        run_callback = true;
                    }
                }

                if (run_callback)
                {
                    if (exception == nullptr)
                    {
                        callback(transport, nullptr);
                    }
                    else
                    {
                        callback({}, exception);
                    }
                }
            });
    }

    void connection_impl::send_connect_request(const std::shared_ptr<transport>& transport, const std::string& url, std::function<void(std::exception_ptr)> callback)
    {
        auto logger = m_logger;
        auto query_string = "id=" + m_connection_token;
        auto connect_url = url_builder::build_connect(url, transport->get_transport_type(), query_string);

        transport->start(connect_url, transfer_format::text, [callback, logger](std::exception_ptr exception)
            mutable {
                try
                {
                    if (exception != nullptr)
                    {
                        std::rethrow_exception(exception);
                    }
                    callback(nullptr);
                }
                catch (const std::exception& e)
                {
                    logger.log(
                        trace_level::errors,
                        std::string("transport could not connect due to: ")
                            .append(e.what()));

                    callback(exception);
                }
            });
    }

    void connection_impl::process_response(std::string&& response)
    {
        m_logger.log(trace_level::messages,
            std::string("processing message: ").append(response));

        invoke_message_received(std::move(response));
    }

    void connection_impl::invoke_message_received(std::string&& message)
    {
        try
        {
            m_message_received(std::move(message));
        }
        catch (const std::exception &e)
        {
            m_logger.log(
                trace_level::errors,
                std::string("message_received callback threw an exception: ")
                .append(e.what()));
        }
        catch (...)
        {
            m_logger.log(trace_level::errors, "message_received callback threw an unknown exception");
        }
    }

    void connection_impl::send(const std::string& data, std::function<void(std::exception_ptr)> callback) noexcept
    {
        // To prevent an (unlikely) condition where the transport is nulled out after we checked the connection_state
        // and before sending data we store the pointer in the local variable. In this case `send()` will throw but
        // we won't crash.
        auto transport = m_transport;

        const auto connection_state = get_connection_state();
        if (connection_state != signalr::connection_state::connected || !transport)
        {
            callback(std::make_exception_ptr(signalr_exception(
                std::string("cannot send data when the connection is not in the connected state. current connection state: ")
                    .append(translate_connection_state(connection_state)))));
            return;
        }

        auto logger = m_logger;

        logger.log(trace_level::info, std::string("sending data: ").append(data));

        transport->send(data, [logger, callback](std::exception_ptr exception)
            mutable {
                try
                {
                    if (exception != nullptr)
                    {
                        std::rethrow_exception(exception);
                    }
                    callback(nullptr);
                }
                catch (const std::exception &e)
                {
                    logger.log(
                        trace_level::errors,
                        std::string("error sending data: ")
                        .append(e.what()));

                    callback(exception);
                }
            });
    }

    void connection_impl::stop(std::function<void(std::exception_ptr)> callback) noexcept
    {
        m_logger.log(trace_level::info, "stopping connection");

        shutdown(callback);
    }

    // This function is called from the dtor so you must not use `shared_from_this` here (it will throw).
    void connection_impl::shutdown(std::function<void(std::exception_ptr)> callback)
    {
        {
            std::lock_guard<std::mutex> lock(m_stop_lock);
            m_logger.log(trace_level::info, "acquired lock in shutdown()");

            const auto current_state = get_connection_state();
            if (current_state == connection_state::disconnected)
            {
                callback(nullptr);
                return;
            }

            if (current_state == connection_state::disconnecting)
            {
                // canceled task will be returned if `stop` was called while another `stop` was already in progress.
                // This is to prevent from resetting the `m_transport` in the upstream callers because doing so might
                // affect the other invocation which is using it.
                callback(std::make_exception_ptr(canceled_exception()));
                return;
            }

            // we request a cancellation of the ongoing start (if any) and wait until it is canceled
            m_disconnect_cts->cancel();

            while (m_start_completed_event.wait(60000) != 0)
            {
                m_logger.log(trace_level::errors,
                    "internal error - stopping the connection is still waiting for the start operation to finish which should have already finished or timed out");
            }

            // at this point we are either in the connected or disconnected state. If we are in the disconnected state
            // we must break because the transport has already been nulled out.
            if (m_connection_state == connection_state::disconnected)
            {
                callback(nullptr);
                return;
            }

            assert(m_connection_state == connection_state::connected);

            change_state(connection_state::disconnecting);
        }

        m_transport->stop(callback);
    }

    // do not use `shared_from_this` as it can be called via the destructor
    void connection_impl::stop_connection(std::exception_ptr error)
    {
        {
            // the lock prevents a race where the user calls `stop` on a disconnected connection and calls `start`
            // on a different thread at the same time. In this case we must not null out the transport if we are
            // not in the `disconnecting` state to not affect the 'start' invocation.
            std::lock_guard<std::mutex> lock(m_stop_lock);

            if (m_connection_state == connection_state::disconnected)
            {
                m_logger.log(trace_level::info, "Stopping was ignored because the connection is already in the disconnected state.");
                return;
            }

            change_state(connection_state::disconnected);
            m_transport = nullptr;
        }

        if (error)
        {
            try
            {
                std::rethrow_exception(error);
            }
            catch (const std::exception & ex)
            {
                m_logger.log(trace_level::errors, std::string("Connection closed with error: ").append(ex.what()));
            }
        }
        else
        {
            m_logger.log(trace_level::info, "Connection closed.");
        }

        try
        {
            m_disconnected();
        }
        catch (const std::exception & e)
        {
            m_logger.log(
                trace_level::errors,
                std::string("disconnected callback threw an exception: ")
                .append(e.what()));
        }
        catch (...)
        {
            m_logger.log(
                trace_level::errors,
                std::string("disconnected callback threw an unknown exception"));
        }
    }

    connection_state connection_impl::get_connection_state() const noexcept
    {
        return m_connection_state.load();
    }

    std::string connection_impl::get_connection_id() const noexcept
    {
        if (m_connection_state.load() == connection_state::connecting)
        {
            return "";
        }

        return m_connection_id;
    }

    void connection_impl::set_message_received(const std::function<void(std::string&&)>& message_received)
    {
        ensure_disconnected("cannot set the callback when the connection is not in the disconnected state. ");
        m_message_received = message_received;
    }

    void connection_impl::set_client_config(const signalr_client_config& config)
    {
        ensure_disconnected("cannot set client config when the connection is not in the disconnected state. ");
        m_signalr_client_config = config;
    }

    void connection_impl::set_disconnected(const std::function<void()>& disconnected)
    {
        ensure_disconnected("cannot set the disconnected callback when the connection is not in the disconnected state. ");
        m_disconnected = disconnected;
    }

    void connection_impl::ensure_disconnected(const std::string& error_message) const
    {
        const auto state = get_connection_state();
        if (state != connection_state::disconnected)
        {
            throw signalr_exception(
                error_message + "current connection state: " + translate_connection_state(state));
        }
    }

    bool connection_impl::change_state(connection_state old_state, connection_state new_state)
    {
        if (m_connection_state.compare_exchange_strong(old_state, new_state, std::memory_order_seq_cst))
        {
            handle_connection_state_change(old_state, new_state);
            return true;
        }

        return false;
    }

    connection_state connection_impl::change_state(connection_state new_state)
    {
        auto old_state = m_connection_state.exchange(new_state);
        if (old_state != new_state)
        {
            handle_connection_state_change(old_state, new_state);
        }

        return old_state;
    }

    void connection_impl::handle_connection_state_change(connection_state old_state, connection_state new_state)
    {
        m_logger.log(
            trace_level::state_changes,
            translate_connection_state(old_state)
            .append(" -> ")
            .append(translate_connection_state(new_state)));

        // Words of wisdom (if we decide to add a state_changed callback and invoke it from here):
        // "Be extra careful when you add this callback, because this is sometimes being called with the m_stop_lock.
        // This could lead to interesting problems.For example, you could run into a segfault if the connection is
        // stopped while / after transitioning into the connecting state."
    }

    std::string connection_impl::translate_connection_state(connection_state state)
    {
        switch (state)
        {
        case connection_state::connecting:
            return "connecting";
        case connection_state::connected:
            return "connected";
        case connection_state::disconnecting:
            return "disconnecting";
        case connection_state::disconnected:
            return "disconnected";
        default:
            assert(false);
            return "(unknown)";
        }
    }
}
