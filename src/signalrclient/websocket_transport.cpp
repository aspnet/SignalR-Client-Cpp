// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#include "stdafx.h"
#include "websocket_transport.h"
#include "logger.h"
#include "signalrclient/signalr_exception.h"
#include "base_uri.h"

#pragma warning (push)
#pragma warning (disable : 5204 4355)
#include <future>
#pragma warning (pop)

namespace signalr
{
    std::shared_ptr<signalr::transport> websocket_transport::create(const std::function<std::shared_ptr<websocket_client>(const signalr_client_config&)>& websocket_client_factory,
        const signalr_client_config& signalr_client_config, const logger& logger)
    {
        return std::shared_ptr<transport>(
            new websocket_transport(websocket_client_factory, signalr_client_config, logger));
    }

    websocket_transport::websocket_transport(const std::function<std::shared_ptr<websocket_client>(const signalr_client_config&)>& websocket_client_factory,
        const signalr_client_config& signalr_client_config, const logger& logger)
        : transport(logger), m_websocket_client_factory(websocket_client_factory), m_process_response_callback([](std::string, std::exception_ptr) {}),
        m_close_callback([](std::exception_ptr) {}), m_signalr_client_config(signalr_client_config),
        m_disconnected(true), m_receive_loop_task(std::make_shared<cancellation_token_source>())
    {
        // we use this cts to check if the receive loop is running so it should be
        // initially canceled to indicate that the receive loop is not running
        m_receive_loop_task->cancel();
    }

    websocket_transport::~websocket_transport()
    {
        try
        {
            std::shared_ptr<std::promise<void>> promise = std::make_shared<std::promise<void>>();
            stop([promise](std::exception_ptr) { promise->set_value(); });
            promise->get_future().get();
        }
        catch (...) // must not throw from the destructor
        {}
    }

    transport_type websocket_transport::get_transport_type() const noexcept
    {
        return transport_type::websockets;
    }

    // Note that the connection assumes that the error callback won't be fired when the result is being processed. This
    // may no longer be true when we replace the `receive_loop` with "on_message_received" and "on_close" events if they
    // can be fired on different threads in which case we will have to lock before setting groups token and message id.
    void websocket_transport::receive_loop()
    {
        auto this_transport = shared_from_this();
        auto logger = this_transport->m_logger;

        // Passing the `std::weak_ptr<websocket_transport>` prevents from a memory leak where we would capture the shared_ptr to
        // the transport in the continuation lambda and as a result as long as the loop runs the ref count would never get to
        // zero. Now we capture the weak pointer and get the shared pointer only when the continuation runs so the ref count is
        // incremented when the shared pointer is acquired and then decremented when it goes out of scope of the continuation.
        auto weak_transport = std::weak_ptr<websocket_transport>(this_transport);

        auto websocket_client = this_transport->safe_get_websocket_client();
        auto weak_websocket_client = std::weak_ptr<signalr::websocket_client>(websocket_client);
        auto receive_loop_task = m_receive_loop_task;

        // There are two cases when we exit the loop. The first case is implicit - we pass the cancellation_token_source
        // to `then` (note this is after the lambda body) and if the token is cancelled the continuation will not
        // run at all. The second - explicit - case happens if the token gets cancelled after the continuation has
        // been started in which case we just stop the loop by not scheduling another receive task.
        websocket_client->receive([weak_transport, logger, receive_loop_task, weak_websocket_client](std::string message, std::exception_ptr exception)
            {
                auto transport = weak_transport.lock();

                // transport can be null if a websocket transport specific test doesn't call and wait for stop and relies on the destructor, if that happens update the test to call and wait for stop.
                // stop waits for the receive loop to complete so the transport should never be null
                assert(transport != nullptr);

                bool disconnected;
                {
                    std::lock_guard<std::mutex> lock(transport->m_start_stop_lock);
                    disconnected = transport->m_disconnected;
                }
                if (disconnected)
                {
                    // stop has been called, tell it the receive loop is done and return
                    receive_loop_task->cancel();
                    return;
                }

                if (exception != nullptr)
                {
                    try
                    {
                        std::rethrow_exception(exception);
                    }
                    catch (const std::exception & e)
                    {
                        logger.log(
                            trace_level::error,
                            std::string("[websocket transport] error receiving response from websocket: ")
                            .append(e.what()));
                    }
                    catch (...)
                    {
                        logger.log(
                            trace_level::error,
                            "[websocket transport] unknown error occurred when receiving response from websocket");

                        exception = std::make_exception_ptr(signalr_exception("unknown error"));
                    }

                    {
                        std::lock_guard<std::mutex> lock(transport->m_start_stop_lock);
                        disconnected = transport->m_disconnected;
                        // prevent transport.stop() from doing anything, we'll handle the close logic here (we can't guarantee the close callback will only be called once otherwise)
                        // this could happen if there was a transport error at the same time someone called stop on the connection
                        transport->m_disconnected = true;
                    }
                    if (disconnected)
                    {
                        // stop has been called, tell it the receive loop is done and return
                        receive_loop_task->cancel();
                        return;
                    }
                    receive_loop_task->cancel();

                    std::promise<void> promise;
                    auto client = weak_websocket_client.lock();
                    if (!client)
                    {
                        // this should not be hit
                        // we wait for the receive loop to complete before completing stop (which will then destruct the transport and client)
                        logger.log(trace_level::critical,
                            "[websocket transport] websocket client has been destructed before the receive loop completes, this is a bug");
                        assert(client != nullptr);
                    }

                    // because transport.stop won't be called we need to stop the underlying client and invoke the transports close callback
                    client->stop([&promise](std::exception_ptr exception)
                    {
                        if (exception != nullptr)
                        {
                            promise.set_exception(exception);
                        }
                        else
                        {
                            promise.set_value();
                        }
                    });

                    try
                    {
                        promise.get_future().get();
                    }
                    // We prefer the outer exception bubbling up to the user
                    // REVIEW: log here?
                    catch (...) {}

                    transport->m_close_callback(exception);

                    return;
                }

                transport->m_process_response_callback(message, nullptr);

                {
                    std::lock_guard<std::mutex> lock(transport->m_start_stop_lock);
                    disconnected = transport->m_disconnected;
                }

                if (!disconnected)
                {
                    assert(!receive_loop_task->is_canceled());
                    transport->receive_loop();
                }
                else
                {
                    receive_loop_task->cancel();
                }
            });
    }

    std::shared_ptr<websocket_client> websocket_transport::safe_get_websocket_client()
    {
        {
            const std::lock_guard<std::mutex> lock(m_websocket_client_lock);
            auto websocket_client = m_websocket_client;

            return websocket_client;
        }
    }

    void websocket_transport::start(const std::string& url, std::function<void(std::exception_ptr)> callback) noexcept
    {
        signalr::uri uri(url);
        assert(uri.scheme() == "ws" || uri.scheme() == "wss");

        {
            std::lock_guard<std::mutex> stop_lock(m_start_stop_lock);

            if (!m_disconnected)
            {
                callback(std::make_exception_ptr(signalr_exception("transport already connected")));
                return;
            }

            m_logger.log(trace_level::info,
                std::string("[websocket transport] connecting to: ")
                .append(url));

            auto websocket_client = m_websocket_client_factory(m_signalr_client_config);

            {
                std::lock_guard<std::mutex> client_lock(m_websocket_client_lock);
                m_websocket_client = websocket_client;
            }

            m_disconnected = false;
            m_receive_loop_task->reset();

            auto weak_transport = std::weak_ptr<websocket_transport>(shared_from_this());

            websocket_client->start(url, [weak_transport, callback](std::exception_ptr exception)
                {
                    auto transport = weak_transport.lock();
                    if (!transport)
                    {
                        callback(std::make_exception_ptr(signalr_exception("transport no longer exists")));
                        return;
                    }

                    try
                    {
                        if (exception != nullptr)
                        {
                            std::rethrow_exception(exception);
                        }

                        if (transport->m_disconnected)
                        {
                            throw signalr::canceled_exception();
                        }

                        transport->receive_loop();
                        callback(nullptr);
                    }
                    catch (const std::exception & e)
                    {
                        transport->m_logger.log(
                            trace_level::error,
                            std::string("[websocket transport] exception when connecting to the server: ")
                            .append(e.what()));

                        transport->m_disconnected = true;
                        callback(std::current_exception());
                    }
                });
        }
    }

    void websocket_transport::stop(std::function<void(std::exception_ptr)> callback) noexcept
    {
        std::shared_ptr<websocket_client> websocket_client = nullptr;

        {
            std::lock_guard<std::mutex> lock(m_start_stop_lock);

            if (m_disconnected)
            {
                callback(nullptr);
                return;
            }

            m_disconnected = true;

            websocket_client = safe_get_websocket_client();
        }

        auto logger = m_logger;
        auto close_callback = m_close_callback;
        auto receive_loop_task = m_receive_loop_task;

        m_logger.log(trace_level::debug, "stopping websocket transport");

        websocket_client->stop([logger, callback, close_callback, receive_loop_task](std::exception_ptr exception)
            {
                receive_loop_task->register_callback([logger, callback, close_callback, exception]()
                    {
                        try
                        {
                            if (exception != nullptr)
                            {
                                std::rethrow_exception(exception);
                            }
                            logger.log(trace_level::debug, "websocket transport stopped");
                        }
                        catch (const std::exception& e)
                        {
                            if (logger.is_enabled(trace_level::error))
                            {
                                logger.log(
                                    trace_level::error,
                                    std::string("websocket transport stopped with error: ")
                                    .append(e.what()));
                            }
                        }

                        close_callback(exception);

                        callback(exception);
                    });
            });
    }

    void websocket_transport::on_close(std::function<void(std::exception_ptr)> callback)
    {
        m_close_callback = callback;
    }

    void websocket_transport::on_receive(std::function<void(std::string&&, std::exception_ptr)> callback)
    {
        m_process_response_callback = callback;
    }

    void websocket_transport::send(const std::string& payload, transfer_format transfer_format, std::function<void(std::exception_ptr)> callback) noexcept
    {
        safe_get_websocket_client()->send(payload, transfer_format, [callback](std::exception_ptr exception)
            {
                if (exception != nullptr)
                {
                    callback(exception);
                }
                else
                {
                    callback(nullptr);
                }
            });
    }
}
