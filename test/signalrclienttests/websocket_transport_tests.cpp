// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#include "stdafx.h"
#include "test_utils.h"
#include "trace_log_writer.h"
#include "test_websocket_client.h"
#include "websocket_transport.h"
#include "default_websocket_client.h"
#include "memory_log_writer.h"
#include <future>
#include <atomic>
#include <algorithm>

using namespace signalr;

TEST(websocket_transport_connect, connect_connects_and_starts_receive_loop)
{
    auto connect_called = false;
    auto client = std::make_shared<test_websocket_client>();

    client->set_connect_function([&connect_called](const std::string&, std::function<void(std::exception_ptr)> callback)
    {
        connect_called = true;
        callback(nullptr);
    });

    std::shared_ptr<log_writer> writer(std::make_shared<memory_log_writer>());

    auto ws_transport = websocket_transport::create([&](const signalr_client_config& config)
        {
            client->set_config(config);
            return client;
        }, signalr_client_config{}, logger(writer, trace_level::info));

    auto mre = manual_reset_event<void>();
    ws_transport->start("ws://fakeuri.org/connect?param=42", [&mre](std::exception_ptr exception)
    {
        mre.set(exception);
    });

    mre.get();

    ASSERT_TRUE(connect_called);
    ASSERT_FALSE(client->receive_loop_started.wait(5000));

    auto log_entries = std::dynamic_pointer_cast<memory_log_writer>(writer)->get_log_entries();
    ASSERT_FALSE(log_entries.empty());

    auto entry = remove_date_from_log_entry(log_entries[0]);
    ASSERT_EQ("[info     ] [websocket transport] connecting to: ws://fakeuri.org/connect?param=42\n", entry);

    ws_transport->stop([&mre](std::exception_ptr exception)
        {
            mre.set(exception);
        });
    mre.get();
}

TEST(websocket_transport_connect, connect_propagates_exceptions)
{
    auto client = std::make_shared<test_websocket_client>();
    client->set_connect_function([](const std::string&, std::function<void(std::exception_ptr)> callback)
    {
        callback(std::make_exception_ptr(std::runtime_error("connecting failed")));
    });

    auto ws_transport = websocket_transport::create([&](const signalr_client_config& config)
        {
            client->set_config(config);
            return client;
        }, signalr_client_config{}, logger(std::make_shared<trace_log_writer>(), trace_level::none));

    try
    {
        auto mre = manual_reset_event<void>();
        ws_transport->start("ws://fakeuri.org", [&mre](std::exception_ptr exception)
        {
            mre.set(exception);
        });
        mre.get();
        ASSERT_TRUE(false);
    }
    catch (const std::exception &e)
    {
        ASSERT_STREQ("connecting failed", e.what());
    }
}

TEST(websocket_transport_connect, connect_logs_exceptions)
{
    auto client = std::make_shared<test_websocket_client>();
    client->set_connect_function([](const std::string&, std::function<void(std::exception_ptr)> callback)
    {
        callback(std::make_exception_ptr(std::runtime_error("connecting failed")));
    });

    std::shared_ptr<log_writer> writer(std::make_shared<memory_log_writer>());
    auto ws_transport = websocket_transport::create([&](const signalr_client_config& config)
        {
            client->set_config(config);
            return client;
        }, signalr_client_config{}, logger(writer, trace_level::error));

    auto mre = manual_reset_event<void>();
    ws_transport->start("ws://fakeuri.org", [&mre](std::exception_ptr exception)
    {
        mre.set(exception);
    });
    try
    {
        mre.get();
    }
    catch (...) {}

    auto log_entries = std::dynamic_pointer_cast<memory_log_writer>(writer)->get_log_entries();

    ASSERT_FALSE(log_entries.empty());

    auto entry = remove_date_from_log_entry(log_entries[0]);

    ASSERT_EQ(
        "[error    ] [websocket transport] exception when connecting to the server: connecting failed\n",
        entry);
}

TEST(websocket_transport_connect, cannot_call_connect_on_already_connected_transport)
{
    auto client = std::make_shared<test_websocket_client>();
    auto ws_transport = websocket_transport::create([&](const signalr_client_config& config)
        {
            client->set_config(config);
            return client;
        }, signalr_client_config{}, logger(std::make_shared<trace_log_writer>(), trace_level::none));

    auto mre = manual_reset_event<void>();
    ws_transport->start("ws://fakeuri.org", [&mre](std::exception_ptr exception)
    {
        mre.set(exception);
    });
    mre.get();

    try
    {
        ws_transport->start("ws://fakeuri.org", [&mre](std::exception_ptr exception)
        {
            mre.set(exception);
        });
        mre.get();
        ASSERT_TRUE(false);
    }
    catch (const std::exception &e)
    {
        ASSERT_STREQ("transport already connected", e.what());
    }

    ws_transport->stop([&mre](std::exception_ptr exception)
        {
            mre.set(exception);
        });
    mre.get();
}

TEST(websocket_transport_connect, can_connect_after_disconnecting)
{
    auto client = std::make_shared<test_websocket_client>();
    auto ws_transport = websocket_transport::create([&](const signalr_client_config& config)
        {
            client->set_config(config);
            return client;
        }, signalr_client_config{}, logger(std::make_shared<trace_log_writer>(), trace_level::none));

    auto mre = manual_reset_event<void>();
    ws_transport->start("ws://fakeuri.org", [&mre](std::exception_ptr exception)
    {
        mre.set(exception);
    });
    mre.get();

    ws_transport->stop([&mre](std::exception_ptr exception)
    {
        mre.set(exception);
    });
    mre.get();

    ws_transport->start("ws://fakeuri.org", [&mre](std::exception_ptr exception)
    {
        mre.set(exception);
    });
    mre.get();

    ws_transport->stop([&mre](std::exception_ptr exception)
        {
            mre.set(exception);
        });
    mre.get();
}

TEST(websocket_transport_send, send_creates_and_sends_websocket_messages)
{
    bool send_called = false;

    auto client = std::make_shared<test_websocket_client>();

    client->set_send_function([&send_called](const std::string&, std::function<void(std::exception_ptr)> callback)
    {
        send_called = true;
        callback(nullptr);
    });

    auto ws_transport = websocket_transport::create([&](const signalr_client_config& config)
        {
            client->set_config(config);
            return client;
        }, signalr_client_config{}, logger(std::make_shared<trace_log_writer>(), trace_level::none));

    auto mre = manual_reset_event<void>();
    ws_transport->start("ws://url", [&mre](std::exception_ptr exception)
    {
        mre.set(exception);
    });
    mre.get();

    ws_transport->send("ABC", transfer_format::text, [&mre](std::exception_ptr exception)
    {
        mre.set(exception);
    });
    mre.get();

    ASSERT_TRUE(send_called);

    ws_transport->stop([&mre](std::exception_ptr exception)
        {
            mre.set(exception);
        });
    mre.get();
}

TEST(websocket_transport_disconnect, disconnect_closes_websocket)
{
    bool close_called = false;

    auto client = std::make_shared<test_websocket_client>();

    client->set_close_function([&close_called](std::function<void(std::exception_ptr)> callback)
    {
        close_called = true;
        callback(nullptr);
    });

    auto ws_transport = websocket_transport::create([&](const signalr_client_config& config)
        {
            client->set_config(config);
            return client;
        }, signalr_client_config{}, logger(std::make_shared<trace_log_writer>(), trace_level::none));

    auto mre = manual_reset_event<void>();
    ws_transport->start("ws://url", [&mre](std::exception_ptr exception)
    {
        mre.set(exception);
    });
    mre.get();

    ws_transport->stop([&mre](std::exception_ptr exception)
    {
        mre.set(exception);
    });
    mre.get();

    ASSERT_TRUE(close_called);
}

TEST(websocket_transport_stop, propogates_exception_from_close)
{
    auto client = std::make_shared<test_websocket_client>();

    bool close_called = false;
    client->set_close_function([&close_called](std::function<void(std::exception_ptr)> callback)
    {
        close_called = true;
        callback(std::make_exception_ptr(std::exception()));
    });

    auto ws_transport = websocket_transport::create([&](const signalr_client_config& config)
        {
            client->set_config(config);
            return client;
        }, signalr_client_config{}, logger(std::make_shared<trace_log_writer>(), trace_level::none));

    auto mre = manual_reset_event<void>();
    ws_transport->start("ws://url", [&mre](std::exception_ptr exception)
    {
        mre.set(exception);
    });
    mre.get();

    ws_transport->stop([&mre](std::exception_ptr exception)
    {
        mre.set(exception);
    });
    try
    {
        mre.get();
        ASSERT_TRUE(false);
    }
    catch (...) { }

    ASSERT_TRUE(close_called);
}

TEST(websocket_transport_disconnect, disconnect_logs_exceptions)
{
    auto client = std::make_shared<test_websocket_client>();
    client->set_close_function([](std::function<void(std::exception_ptr)> callback)
    {
        callback(std::make_exception_ptr(std::runtime_error("connection closing failed")));
    });

    std::shared_ptr<log_writer> writer(std::make_shared<memory_log_writer>());

    auto ws_transport = websocket_transport::create([&](const signalr_client_config& config)
        {
            client->set_config(config);
            return client;
        }, signalr_client_config{}, logger(writer, trace_level::error));

    auto mre = manual_reset_event<void>();
    ws_transport->start("ws://url", [&mre](std::exception_ptr exception)
    {
        mre.set(exception);
    });
    mre.get();

    ws_transport->stop([&mre](std::exception_ptr exception)
    {
        mre.set(exception);
    });

    try
    {
        mre.get();
        ASSERT_TRUE(false);
    }
    catch (...) {}

    auto log_entries = std::dynamic_pointer_cast<memory_log_writer>(writer)->get_log_entries();

    ASSERT_FALSE(log_entries.empty());

    // disconnect cancels the receive loop by setting the cancellation token source to cancelled which results in writing
    // to the log. Exceptions from close are also logged but this happens on a different thread. As a result the order
    // of messages in the log is not deterministic and therefore we just use the "contains" idiom to find the message.
    ASSERT_NE(std::find_if(log_entries.begin(), log_entries.end(), [](const std::string& entry)
        {
            return remove_date_from_log_entry(entry) ==
                "[error    ] websocket transport stopped with error: connection closing failed\n";
        }),
        log_entries.end());
}

TEST(websocket_transport_disconnect, receive_not_called_after_disconnect)
{
    auto client = std::make_shared<test_websocket_client>();

    client->set_close_function([](std::function<void(std::exception_ptr)> callback)
    {
        callback(nullptr);
    });

    auto ws_transport = websocket_transport::create([&](const signalr_client_config& config)
        {
            client->set_config(config);
            return client;
        }, signalr_client_config{}, logger(std::make_shared<trace_log_writer>(), trace_level::none));

    auto mre = manual_reset_event<void>();
    ws_transport->start("ws://fakeuri.org", [&mre](std::exception_ptr)
    {
        mre.set();
    });
    mre.get();

    ASSERT_FALSE(client->receive_loop_started.wait(5000));

    ws_transport->stop([&mre](std::exception_ptr exception)
    {
        mre.set(exception);
    });
    mre.get();

    ws_transport->start("ws://fakeuri.org", [&mre](std::exception_ptr exception)
    {
        mre.set(exception);
    });
    mre.get();
    ASSERT_FALSE(client->receive_loop_started.wait(5000));

    ws_transport->stop([&mre](std::exception_ptr exception)
    {
        mre.set(exception);
    });
    mre.get();

    ASSERT_EQ(2, client->receive_count);
}

TEST(websocket_transport_disconnect, disconnect_is_no_op_if_transport_not_started)
{
    auto client = std::make_shared<test_websocket_client>();

    auto close_called = false;

    client->set_close_function([&close_called](std::function<void(std::exception_ptr)> callback)
    {
        close_called = true;
        callback(nullptr);
    });

    auto ws_transport = websocket_transport::create([&](const signalr_client_config& config)
        {
            client->set_config(config);
            return client;
        }, signalr_client_config{}, logger(std::make_shared<trace_log_writer>(), trace_level::none));

    auto mre = manual_reset_event<void>();
    ws_transport->stop([&mre](std::exception_ptr exception)
    {
        mre.set(exception);
    });
    mre.get();

    ASSERT_FALSE(close_called);
}

template<typename T>
void receive_loop_logs_exception_runner(const T& e, const std::string& expected_message, trace_level trace_level);

TEST(websocket_transport_receive_loop, receive_loop_logs_std_exception)
{
    receive_loop_logs_exception_runner(
        std::runtime_error("exception"),
        "[error    ] [websocket transport] error receiving response from websocket: exception\n",
        trace_level::error);
}

template<typename T>
void receive_loop_logs_exception_runner(const T& e, const std::string& expected_message, trace_level trace_level)
{
    auto client = std::make_shared<test_websocket_client>();
    std::shared_ptr<log_writer> writer(std::make_shared<memory_log_writer>());

    auto ws_transport = websocket_transport::create([&](const signalr_client_config& config)
        {
            client->set_config(config);
            return client;
        }, signalr_client_config{}, logger(writer, trace_level));

    auto mre = manual_reset_event<void>();
    ws_transport->start("ws://url", [&mre](std::exception_ptr exception)
    {
        mre.set(exception);
    });
    mre.get();

    ASSERT_FALSE(client->receive_loop_started.wait(5000));

    auto close_mre = manual_reset_event<void>();
    ws_transport->on_close([&close_mre](std::exception_ptr exception)
        {
            close_mre.set(exception);
        });

    client->receive_message(std::make_exception_ptr(e));

    try
    {
        close_mre.get();
        ASSERT_TRUE(false);
    }
    catch (const std::exception & ex)
    {
        ASSERT_STREQ(e.what(), ex.what());
    }

    auto log_entries = std::dynamic_pointer_cast<memory_log_writer>(writer)->get_log_entries();

    ASSERT_NE(std::find_if(log_entries.begin(), log_entries.end(),
        [&expected_message](std::string entry) { return remove_date_from_log_entry(entry) == expected_message; }),
        log_entries.end()) << dump_vector(log_entries);
}

TEST(websocket_transport_receive_loop, process_response_callback_called_when_message_received)
{
    auto client = std::make_shared<test_websocket_client>();

    auto process_response_event = std::make_shared<cancellation_token_source>();
    auto msg = std::make_shared<std::string>();

    auto process_response = [msg, process_response_event](const std::string& message, std::exception_ptr exception)
    {
        ASSERT_FALSE(exception);
        *msg = message;
        process_response_event->cancel();
    };

    auto ws_transport = websocket_transport::create([&](const signalr_client_config& config)
        {
            client->set_config(config);
            return client;
        }, signalr_client_config{}, logger(std::make_shared<trace_log_writer>(), trace_level::none));
    ws_transport->on_receive(process_response);

    auto mre = manual_reset_event<void>();
    ws_transport->start("ws://fakeuri.org", [&mre](std::exception_ptr exception)
    {
        mre.set(exception);
    });
    mre.get();

    client->receive_message("msg");

    process_response_event->wait(1000);

    ASSERT_EQ(std::string("msg"), *msg);

    ws_transport->stop([&mre](std::exception_ptr exception)
        {
            mre.set(exception);
        });
    mre.get();
}

TEST(websocket_transport_receive_loop, error_callback_called_when_exception_thrown)
{
    auto client = std::make_shared<test_websocket_client>();

    auto close_invoked = std::make_shared<bool>(false);
    client->set_close_function([close_invoked](std::function<void(std::exception_ptr)> callback)
    {
        *close_invoked = true;
        callback(nullptr);
    });

    auto error_event = std::make_shared<cancellation_token_source>();
    auto exception_msg = std::make_shared<std::string>();

    auto error_callback = [exception_msg, error_event](std::exception_ptr exception)
    {
        try
        {
            std::rethrow_exception(exception);
        }
        catch (const std::exception& e)
        {
            *exception_msg = e.what();
        }
        error_event->cancel();
    };

    auto ws_transport = websocket_transport::create([&](const signalr_client_config& config)
        {
            client->set_config(config);
            return client;
        }, signalr_client_config{}, logger(std::make_shared<trace_log_writer>(), trace_level::none));
    ws_transport->on_close(error_callback);

    auto mre = manual_reset_event<void>();
    ws_transport->start("ws://fakeuri.org", [&mre](std::exception_ptr exception)
    {
        mre.set(exception);
    });
    mre.get();

    client->receive_message(std::make_exception_ptr(std::runtime_error("error")));

    error_event->wait(1000);

    ASSERT_STREQ("error", exception_msg->c_str());
    ASSERT_TRUE(*close_invoked);
}

TEST(websocket_transport_get_transport_type, get_transport_type_returns_websockets)
{
    auto ws_transport = websocket_transport::create(
        [](const signalr_client_config& config)
        {
            auto client = std::make_shared<test_websocket_client>();
            client->set_config(config);
            return client;
        }, signalr_client_config{},
        logger(std::make_shared<trace_log_writer>(), trace_level::none));

    ASSERT_EQ(transport_type::websockets, ws_transport->get_transport_type());
}

class throwing_websocket_client : public websocket_client
{
public:
    virtual void start(const std::string& url, std::function<void(std::exception_ptr)> callback) noexcept
    {
        callback(nullptr);
    }

    virtual void stop(std::function<void(std::exception_ptr)> callback) noexcept
    {
        m_stop = true;
        callback(nullptr);
    }

    virtual void send(const std::string& payload, signalr::transfer_format transfer_format, std::function<void(std::exception_ptr)> callback) noexcept
    {
        callback(nullptr);
    }

    virtual void receive(std::function<void(const std::string&, std::exception_ptr)> callback)
    {
        std::thread([callback, this]()
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                std::exception_ptr exception = nullptr;
                if (m_stop == true)
                {
                    exception = std::make_exception_ptr(std::runtime_error("throw from receive"));
                }
                callback("", exception);
            }).detach();
    }
private:
    bool m_stop;
};

TEST(stop, error_from_receive_on_stop_properly_closes)
{
    auto ws_transport = websocket_transport::create(
        [](const signalr_client_config& config)
        {
            auto client = std::make_shared<throwing_websocket_client>();
            return client;
        }, signalr_client_config{},
        logger(std::make_shared<trace_log_writer>(), trace_level::none));

    std::atomic_int stop_called{ 0 };
    std::atomic_int on_close_called{ 0 };
    auto mre = manual_reset_event<void>();
    ws_transport->on_close([&mre, &on_close_called](std::exception_ptr ex)
        {
            on_close_called++;
            mre.set(ex);
        });
    ws_transport->start("ws://fakeuri.org", [](std::exception_ptr) {});
    auto stop_mre = manual_reset_event<void>();
    ws_transport->stop([&stop_mre, &stop_called](std::exception_ptr ex)
        {
            stop_called++;
            stop_mre.set(ex);
        });
    mre.get();
    stop_mre.get();
    ASSERT_EQ(1, on_close_called.load());
    ASSERT_EQ(1, stop_called.load());
}

class caching_websocket_client : public websocket_client
{
public:
    virtual void start(const std::string& url, std::function<void(std::exception_ptr)> callback) noexcept
    {
        m_start_callback = callback;
        callback(nullptr);
    }

    virtual void stop(std::function<void(std::exception_ptr)> callback) noexcept
    {
        m_stop_callback = callback;
        callback(nullptr);
    }

    virtual void send(const std::string& payload, signalr::transfer_format transfer_format, std::function<void(std::exception_ptr)> callback) noexcept
    {
        callback(nullptr);
    }

    virtual void receive(std::function<void(const std::string&, std::exception_ptr)> callback)
    {
        m_receive_callback = callback;
        std::thread([callback]()
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                callback("", nullptr);
            }).detach();
    }
private:
    std::function<void(std::exception_ptr)> m_start_callback;
    std::function<void(std::exception_ptr)> m_stop_callback;
    std::function<void(const std::string&, std::exception_ptr)> m_receive_callback;
};

// validates that a 3rd party websocket client implementation wont cause a memory leak if it caches the callbacks passed from the websocket_transport
TEST(websocket_client_custom_impl, caching_callbacks_does_not_leak)
{
    auto ws_transport = websocket_transport::create(
        [](const signalr_client_config& config)
        {
            auto client = std::make_shared<caching_websocket_client>();
            return client;
        }, signalr_client_config{},
        logger(std::make_shared<trace_log_writer>(), trace_level::none));

    ws_transport->start("ws://fakeuri.org", [](std::exception_ptr) {});
    auto mre = manual_reset_event<void>();
    ws_transport->stop([&mre](std::exception_ptr exception)
    {
        mre.set(exception);
    });
    mre.get();
}