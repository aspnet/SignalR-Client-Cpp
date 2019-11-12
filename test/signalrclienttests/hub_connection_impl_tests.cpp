// Copyright (c) .NET Foundation. All rights reserved.
// Licensed under the Apache License, Version 2.0. See License.txt in the project root for license information.

#include "stdafx.h"
#include "test_utils.h"
#include "test_transport_factory.h"
#include "test_http_client.h"
#include "hub_connection_impl.h"
#include "memory_log_writer.h"
#include "signalrclient/hub_exception.h"
#include "signalrclient/signalr_exception.h"

using namespace signalr;

std::shared_ptr<hub_connection_impl> create_hub_connection(std::shared_ptr<websocket_client> websocket_client = create_test_websocket_client(),
    std::shared_ptr<log_writer> log_writer = std::make_shared<memory_log_writer>(), trace_level trace_level = trace_level::all)
{
    return hub_connection_impl::create(create_uri(), trace_level, log_writer,
        create_test_http_client(), std::make_unique<test_transport_factory>(websocket_client));
}

int default_receive_func(int number, std::function<void(std::string, std::exception_ptr)> callback)
{
    std::string responses[]
    {
        "{ }\x1e",
        "{\"type\":6}\x1e"
    };

    callback(responses[number], nullptr);
    number = std::min(1, number + 1);
    return number;
}

TEST(url, negotiate_appended_to_url)
{
    std::string base_urls[] = { "http://fakeuri", "http://fakeuri/" };

    for (const auto& base_url : base_urls)
    {
        std::string requested_url;
        auto http_client = std::make_unique<test_http_client>([&requested_url](const std::string& url, http_request)
        {
            requested_url = url;
            return http_response{ 404, "" };
        });

        auto hub_connection = hub_connection_impl::create(base_url, trace_level::none,
            std::make_shared<memory_log_writer>(), std::move(http_client),
            std::make_unique<test_transport_factory>(create_test_websocket_client()));

        auto mre = manual_reset_event<void>();
        hub_connection->start([&mre](std::exception_ptr exception)
        {
            mre.set(exception);
        });

        try
        {
            mre.get();
            ASSERT_TRUE(false);
        }
        catch (const std::exception&) {}

        ASSERT_EQ("http://fakeuri/negotiate?negotiateVersion=1", requested_url);
    }
}

TEST(start, start_starts_connection)
{
    int number = 0;
    auto websocket_client = create_test_websocket_client(
        /* receive function */ [number](std::function<void(std::string, std::exception_ptr)> callback) mutable { number = default_receive_func(number, callback); });
    auto hub_connection = create_hub_connection(websocket_client);

    auto mre = manual_reset_event<void>();
    hub_connection->start([&mre](std::exception_ptr exception)
    {
        mre.set(exception);
    });

    mre.get();

    ASSERT_EQ(connection_state::connected, hub_connection->get_connection_state());
}

TEST(start, start_sends_handshake)
{
    int number = 0;
    auto message = std::make_shared<std::string>();
    auto websocket_client = create_test_websocket_client(
        /* receive function */ [number](std::function<void(std::string, std::exception_ptr)> callback) mutable { number = default_receive_func(number, callback); },
        /* send function */ [message](const std::string& msg, std::function<void(std::exception_ptr)> callback) { *message = msg; callback(nullptr); });
    auto hub_connection = create_hub_connection(websocket_client);

    auto mre = manual_reset_event<void>();
    hub_connection->start([&mre](std::exception_ptr exception)
    {
        mre.set(exception);
    });

    mre.get();

    ASSERT_EQ("{\"protocol\":\"json\",\"version\":1}\x1e", *message);

    ASSERT_EQ(connection_state::connected, hub_connection->get_connection_state());
}

TEST(start, start_waits_for_handshake_response)
{
    int number = 0;
    pplx::task_completion_event<void> tce;
    pplx::task_completion_event<void> tceWaitForSend;
    auto websocket_client = create_test_websocket_client(
        /* receive function */ [tce, tceWaitForSend, number](std::function<void(std::string, std::exception_ptr)> callback)
        mutable {
            tceWaitForSend.set();
            pplx::task<void>(tce).get();
            number = default_receive_func(number, callback);
        });
    auto hub_connection = create_hub_connection(websocket_client);

    auto mre = manual_reset_event<void>();
    auto done = false;
    hub_connection->start([&mre, &done](std::exception_ptr exception)
    {
        done = true;
        mre.set(exception);
    });

    pplx::task<void>(tceWaitForSend).get();
    ASSERT_FALSE(done);
    tce.set();
    mre.get();

    ASSERT_EQ(connection_state::connected, hub_connection->get_connection_state());
}

TEST(start, start_fails_for_handshake_response_with_error)
{
    auto websocket_client = create_test_websocket_client(
        /* receive function */ [](std::function<void(std::string, std::exception_ptr)> callback) { callback("{\"error\":\"bad things\"}\x1e", nullptr); });
    auto hub_connection = create_hub_connection(websocket_client);

    auto mre = manual_reset_event<void>();
    hub_connection->start([&mre](std::exception_ptr exception)
    {
        mre.set(exception);
    });

    try
    {
        mre.get();
        ASSERT_TRUE(false);
    }
    catch (const std::exception& ex)
    {
        ASSERT_STREQ("Received an error during handshake: bad things", ex.what());
    }

    ASSERT_EQ(connection_state::disconnected, hub_connection->get_connection_state());
}

TEST(start, start_fails_if_stop_called_before_handshake_response)
{
    pplx::task_completion_event<std::string> tce;
    pplx::task_completion_event<void> tceWaitForSend;
    auto websocket_client = create_test_websocket_client(
        /* receive function */ [tce](std::function<void(std::string, std::exception_ptr)> callback)
    {
        auto str = pplx::task<std::string>(tce).get();
        callback(str, nullptr);
    },
        /* send function */ [tceWaitForSend](const std::string &, std::function<void(std::exception_ptr)> callback)
        {
            tceWaitForSend.set();
            callback(nullptr);
        });
    auto hub_connection = create_hub_connection(websocket_client);

    auto mre = manual_reset_event<void>();
    hub_connection->start([&mre](std::exception_ptr exception)
    {
        mre.set(exception);
    });

    pplx::task<void>(tceWaitForSend).get();
    hub_connection->stop([](std::exception_ptr) {});

    try
    {
        mre.get();
        ASSERT_TRUE(false);
    }
    catch (const std::exception& ex)
    {
        ASSERT_STREQ("connection closed while handshake was in progress.", ex.what());
    }

    ASSERT_EQ(connection_state::disconnected, hub_connection->get_connection_state());

    // Release receive function
    tce.set("");
}

TEST(stop, stop_stops_connection)
{
    int number = 0;
    auto websocket_client = create_test_websocket_client(
        /* receive function */ [number](std::function<void(std::string, std::exception_ptr)> callback) mutable { number = default_receive_func(number, callback); });
    auto hub_connection = create_hub_connection(websocket_client);

    auto mre = manual_reset_event<void>();
    hub_connection->start([&mre](std::exception_ptr exception)
    {
        mre.set(exception);
    });

    mre.get();

    hub_connection->stop([&mre](std::exception_ptr exception)
    {
        mre.set(exception);
    });

    mre.get();

    ASSERT_EQ(connection_state::disconnected, hub_connection->get_connection_state());
}

TEST(stop, disconnected_callback_called_when_hub_connection_stops)
{
    int number = 0;
    auto websocket_client = create_test_websocket_client(
        /* receive function */ [number](std::function<void(std::string, std::exception_ptr)> callback) mutable { number = default_receive_func(number, callback); });
    auto hub_connection = create_hub_connection(websocket_client);

    auto disconnected_invoked = false;
    hub_connection->set_disconnected([&disconnected_invoked]() { disconnected_invoked = true; });

    auto mre = manual_reset_event<void>();
    hub_connection->start([&mre](std::exception_ptr exception)
    {
        mre.set(exception);
    });

    mre.get();

    hub_connection->stop([&mre](std::exception_ptr exception)
    {
        mre.set(exception);
    });

    mre.get();

    ASSERT_TRUE(disconnected_invoked);
}

TEST(stop, connection_stopped_when_going_out_of_scope)
{
    std::shared_ptr<log_writer> writer(std::make_shared<memory_log_writer>());

    {
        int number = 0;
        auto websocket_client = create_test_websocket_client(
            /* receive function */ [number](std::function<void(std::string, std::exception_ptr)> callback) mutable { number = default_receive_func(number, callback); });
        auto hub_connection = create_hub_connection(websocket_client, writer, trace_level::state_changes);

        auto mre = manual_reset_event<void>();
        hub_connection->start([&mre](std::exception_ptr exception)
        {
            mre.set(exception);
        });

        mre.get();
    }

    auto memory_writer = std::dynamic_pointer_cast<memory_log_writer>(writer);

    // The underlying connection_impl will be destroyed when the last reference to shared_ptr holding is released. This can happen
    // on a different thread in which case the dtor will be invoked on a different thread so we need to wait for this
    // to happen and if it does not the test will fail. There is nothing we can block on.
    for (int wait_time_ms = 5; wait_time_ms < 6000 && memory_writer->get_log_entries().size() < 4; wait_time_ms <<= 1)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(wait_time_ms));
    }

    auto log_entries = memory_writer->get_log_entries();
    ASSERT_EQ(4U, log_entries.size()) << dump_vector(log_entries);
    ASSERT_EQ("[state change] disconnected -> connecting\n", remove_date_from_log_entry(log_entries[0]));
    ASSERT_EQ("[state change] connecting -> connected\n", remove_date_from_log_entry(log_entries[1]));
    ASSERT_EQ("[state change] connected -> disconnecting\n", remove_date_from_log_entry(log_entries[2]));
    ASSERT_EQ("[state change] disconnecting -> disconnected\n", remove_date_from_log_entry(log_entries[3]));
}

TEST(stop, stop_cancels_pending_callbacks)
{
    int number = 0;
    auto websocket_client = create_test_websocket_client(
        /* receive function */ [number](std::function<void(std::string, std::exception_ptr)> callback) mutable { number = default_receive_func(number, callback); });

    auto hub_connection = create_hub_connection(websocket_client);
    auto mre = manual_reset_event<void>();
    hub_connection->start([&mre](std::exception_ptr exception)
    {
        mre.set(exception);
    });

    mre.get();

    auto invoke_mre = manual_reset_event<void>();
    hub_connection->invoke("method", signalr::value(signalr::value_type::array), [&invoke_mre](const signalr::value&, std::exception_ptr exception)
    {
        invoke_mre.set(exception);
    });

    hub_connection->stop([&mre](std::exception_ptr exception)
    {
        mre.set(exception);
    });

    mre.get();

    try
    {
        invoke_mre.get();
        ASSERT_TRUE(false); // exception expected but not thrown
    }
    catch (const signalr_exception& e)
    {
        ASSERT_STREQ("connection was stopped before invocation result was received", e.what());
    }
}

TEST(stop, pending_callbacks_finished_if_hub_connections_goes_out_of_scope)
{
    int number = 0;
    auto websocket_client = create_test_websocket_client(
        /* receive function */ [number](std::function<void(std::string, std::exception_ptr)> callback) mutable { number = default_receive_func(number, callback); });

    auto invoke_mre = manual_reset_event<void>();

    {
        auto hub_connection = create_hub_connection(websocket_client);
        auto mre = manual_reset_event<void>();
        hub_connection->start([&mre](std::exception_ptr exception)
        {
            mre.set(exception);
        });

        mre.get();

        hub_connection->invoke("method", signalr::value(signalr::value_type::array), [&invoke_mre](const signalr::value&, std::exception_ptr exception)
        {
            invoke_mre.set(exception);
        });
    }

    try
    {
        invoke_mre.get();
        ASSERT_TRUE(false); // exception expected but not thrown
    }
    catch (const signalr_exception& e)
    {
        ASSERT_STREQ("connection went out of scope before invocation result was received", e.what());
    }
}

TEST(hub_invocation, hub_connection_invokes_users_code_on_hub_invocations)
{
    int call_number = -1;
    auto websocket_client = create_test_websocket_client(
        /* receive function */ [call_number](std::function<void(std::string, std::exception_ptr)> callback)
    mutable {
        std::string responses[]
        {
            "{ }\x1e",
            "{ \"type\": 1, \"target\": \"BROADcast\", \"arguments\": [ \"message\", 1 ] }\x1e"
        };

        call_number = std::min(call_number + 1, 1);

        callback(responses[call_number], nullptr);
    });

    auto hub_connection = create_hub_connection(websocket_client);

    auto payload = std::make_shared<signalr::value>();
    auto on_broadcast_event = std::make_shared<cancellation_token>();
    hub_connection->on("broadCAST", [on_broadcast_event, payload](const signalr::value& message)
    {
        *payload = message;
        on_broadcast_event->cancel();
    });

    auto mre = manual_reset_event<void>();
    hub_connection->start([&mre](std::exception_ptr exception)
    {
        mre.set(exception);
    });

    mre.get();
    ASSERT_FALSE(on_broadcast_event->wait(5000));

    auto array = payload->as_array();
    ASSERT_EQ(2, array.size());
    ASSERT_EQ("message", array[0].as_string());
    ASSERT_EQ(1, array[1].as_double());
}

TEST(hub_invocation, hub_connection_throws_when_missing_arguments_or_target)
{
    int call_number = -1;
    auto websocket_client = create_test_websocket_client(
        /* receive function */ [call_number](std::function<void(std::string, std::exception_ptr)> callback)
        mutable {
            std::string responses[]
            {
                "{ }\x1e",
                "{ \"type\": 1, \"target\": \"broadcast\" }\x1e",
                "{ \"type\": 1, \"arguments\": [] }\x1e",
                "{ \"type\": 1, \"target\": \"broadcast\", \"arguments\": [] }\x1e",
                "{ \"type\": 3 }\x1e",
            };

            call_number = std::min(call_number + 1, 3);

            callback(responses[call_number], nullptr);
        });

    std::shared_ptr<log_writer> writer(std::make_shared<memory_log_writer>());
    auto hub_connection = create_hub_connection(websocket_client, writer, trace_level::errors);

    auto payload = std::make_shared<signalr::value>();
    auto on_broadcast_event = std::make_shared<cancellation_token>();
    hub_connection->on("broadcast", [on_broadcast_event, payload](const signalr::value& message)
        {
            *payload = message;
            on_broadcast_event->cancel();
        });

    auto mre = manual_reset_event<void>();
    hub_connection->start([&mre](std::exception_ptr exception)
        {
            mre.set(exception);
        });

    mre.get();
    ASSERT_FALSE(on_broadcast_event->wait(5000));

    auto array = payload->as_array();
    ASSERT_EQ(0, array.size());

    auto log_entries = std::dynamic_pointer_cast<memory_log_writer>(writer)->get_log_entries();
    ASSERT_TRUE(log_entries.size() == 2);

    auto entry = remove_date_from_log_entry(log_entries[0]);
    ASSERT_EQ("[error       ] error occured when parsing response: Field 'arguments' not found for 'invocation' message. response: { \"type\": 1, \"target\": \"broadcast\" }\x1e\n", entry) << dump_vector(log_entries);

    entry = remove_date_from_log_entry(log_entries[1]);
    ASSERT_EQ("[error       ] error occured when parsing response: Field 'target' not found for 'invocation' message. response: { \"type\": 1, \"arguments\": [] }\x1e\n", entry) << dump_vector(log_entries);
}

TEST(send, creates_correct_payload)
{
    std::string payload;
    bool handshakeReceived = false;
    int number = 0;

    auto websocket_client = create_test_websocket_client(
        /* receive function */ [number](std::function<void(std::string, std::exception_ptr)> callback) mutable { number = default_receive_func(number, callback); },
        /* send function */[&payload, &handshakeReceived](const std::string& m, std::function<void(std::exception_ptr)> callback)
        {
            if (handshakeReceived)
            {
                payload = m;
                callback(nullptr);
                return;
            }
            handshakeReceived = true;
            callback(nullptr);
        });

    auto hub_connection = create_hub_connection(websocket_client);
    auto mre = manual_reset_event<void>();
    hub_connection->start([&mre](std::exception_ptr exception)
    {
        mre.set(exception);
    });

    mre.get();

    hub_connection->send("method", signalr::value(signalr::value_type::array), [&mre](std::exception_ptr exception)
    {
        mre.set(exception);
    });

    mre.get();

    ASSERT_EQ("{\"arguments\":[],\"target\":\"method\",\"type\":1}\x1e", payload);
}

TEST(send, does_not_wait_for_server_response)
{
    int call_number = -1;
    pplx::task_completion_event<void> waitForSend;

    auto websocket_client = create_test_websocket_client(
    /* receive function */ [waitForSend, call_number](std::function<void(std::string, std::exception_ptr)> callback) mutable
    {
        std::string responses[]
        {
            "{ }\x1e",
            "{\"type\":6}\x1e"
        };

        call_number = std::min(call_number + 1, 1);

        if (call_number == 1)
        {
            pplx::task<void>(waitForSend).get();
        }

        callback(responses[call_number], nullptr);
    });

    auto hub_connection = create_hub_connection(websocket_client);

    auto mre = manual_reset_event<void>();
    hub_connection->start([&mre](std::exception_ptr exception)
    {
        mre.set(exception);
    });

    mre.get();

    // wont block waiting for server response
    hub_connection->send("method", signalr::value(signalr::value_type::array), [&mre](std::exception_ptr exception)
    {
        mre.set(exception);
    });
    mre.get();
    waitForSend.set();
}

TEST(send, passing_non_array_arguments_fails)
{
    int call_number = -1;
    pplx::task_completion_event<void> waitForSend;

    auto websocket_client = create_test_websocket_client(
        /* receive function */ [waitForSend, call_number](std::function<void(std::string, std::exception_ptr)> callback) mutable
        {
            std::string responses[]
            {
                "{ }\x1e",
                "{\"type\":6}\x1e"
            };

            call_number = std::min(call_number + 1, 1);

            if (call_number == 1)
            {
                pplx::task<void>(waitForSend).get();
            }

            callback(responses[call_number], nullptr);
        });

    auto hub_connection = create_hub_connection(websocket_client);

    auto mre = manual_reset_event<void>();
    hub_connection->start([&mre](std::exception_ptr exception)
        {
            mre.set(exception);
        });

    mre.get();

    // wont block waiting for server response
    hub_connection->send("method", signalr::value(true), [&mre](std::exception_ptr exception)
        {
            mre.set(exception);
        });

    try
    {
        mre.get();
        ASSERT_TRUE(false);
    }
    catch (const std::exception & ex)
    {
        ASSERT_STREQ("arguments should be an array", ex.what());
    }

    waitForSend.set();
}

TEST(invoke, creates_correct_payload)
{
    std::string payload;
    bool handshakeReceived = false;
    int number = 0;

    auto websocket_client = create_test_websocket_client(
        /* receive function */ [number](std::function<void(std::string, std::exception_ptr)> callback) mutable { number = default_receive_func(number, callback); },
        /* send function */[&payload, &handshakeReceived](const std::string& m, std::function<void(std::exception_ptr)> callback)
    {
        if (handshakeReceived)
        {
            payload = m;
            callback(std::make_exception_ptr(std::runtime_error("error")));
            return;
        }
        handshakeReceived = true;
        callback(nullptr);
    });

    auto hub_connection = create_hub_connection(websocket_client);
    auto mre = manual_reset_event<void>();
    hub_connection->start([&mre](std::exception_ptr exception)
    {
        mre.set(exception);
    });

    mre.get();

    hub_connection->invoke("method", signalr::value(signalr::value_type::array), [&mre](const signalr::value&, std::exception_ptr exception)
    {
        mre.set(exception);
    });

    try
    {
        mre.get();
        ASSERT_TRUE(false);
    }
    catch (...)
    {
        // the invoke is not setup to succeed because it's not needed in this test
    }

    ASSERT_EQ("{\"arguments\":[],\"invocationId\":\"0\",\"target\":\"method\",\"type\":1}\x1e", payload);
}

TEST(invoke, callback_not_called_if_send_throws)
{
    bool handshakeReceived = false;
    int number = 0;
    auto websocket_client = create_test_websocket_client(
        /* receive function */ [number](std::function<void(std::string, std::exception_ptr)> callback) mutable { number = default_receive_func(number, callback); },
        /* send function */[handshakeReceived](const std::string&, std::function<void(std::exception_ptr)> callback) mutable
        {
            if (handshakeReceived)
            {
                callback(std::make_exception_ptr(std::runtime_error("error")));
                return;
            }
            handshakeReceived = true;
            callback(nullptr);
        });

    auto hub_connection = create_hub_connection(websocket_client);
    auto mre = manual_reset_event<void>();
    hub_connection->start([&mre](std::exception_ptr exception)
    {
        mre.set(exception);
    });

    mre.get();

    hub_connection->invoke("method", signalr::value(signalr::value_type::array), [&mre](const signalr::value&, std::exception_ptr exception)
    {
        mre.set(exception);
    });

    try
    {
        mre.get();
        ASSERT_TRUE(false); // exception expected but not thrown
    }
    catch (const std::runtime_error& e)
    {
        ASSERT_STREQ("error", e.what());
    }

    // stop completes all outstanding callbacks so if we did not remove a callback when `invoke_void` failed an
    // unobserved exception exception would be thrown. Note that this would happen on a different thread and would
    // crash the process
    hub_connection->stop([&mre](std::exception_ptr exception)
    {
        mre.set(exception);
    });

    mre.get();
}

TEST(invoke, invoke_returns_value_returned_from_the_server)
{
    auto callback_registered_event = std::make_shared<cancellation_token>();

    int call_number = -1;
    auto websocket_client = create_test_websocket_client(
        /* receive function */ [call_number, callback_registered_event](std::function<void(std::string, std::exception_ptr)> callback)
        mutable {
        std::string responses[]
        {
            "{ }\x1e",
            "{ \"type\": 3, \"invocationId\": \"0\", \"result\": \"abc\" }\x1e"
        };

        call_number = std::min(call_number + 1, 1);

        if (call_number > 0)
        {
            callback_registered_event->wait();
        }

        callback(responses[call_number], nullptr);
    });

    auto hub_connection = create_hub_connection(websocket_client);

    auto mre = manual_reset_event<void>();
    hub_connection->start([&mre](std::exception_ptr exception)
    {
        mre.set(exception);
    });

    mre.get();

    auto invoke_mre = manual_reset_event<signalr::value>();
    hub_connection->invoke("method", signalr::value(signalr::value_type::array), [&invoke_mre](const signalr::value& message, std::exception_ptr exception)
    {
        if (exception)
        {
            invoke_mre.set(exception);
        }
        else
        {
            invoke_mre.set(message);
        }
    });

    callback_registered_event->cancel();

    auto result = invoke_mre.get();

    ASSERT_TRUE(result.is_string());
    ASSERT_EQ("abc", result.as_string());
}

TEST(temporary, test_memory)
{
    for (int i = 0; i < 1000; ++i)
    {
        auto s = signalr::value(std::map<std::string, signalr::value>());
        auto s2 = signalr::value(std::string());
        auto s3 = signalr::value(std::vector<signalr::value>());
        auto s4 = std::move(s3);
    }
}

TEST(invoke, invoke_propagates_errors_from_server_as_hub_exceptions)
{
    auto callback_registered_event = std::make_shared<cancellation_token>();

    int call_number = -1;
    auto websocket_client = create_test_websocket_client(
        /* receive function */ [call_number, callback_registered_event](std::function<void(std::string, std::exception_ptr)> callback)
        mutable {
        std::string responses[]
        {
            "{ }\x1e",
            "{ \"type\": 3, \"invocationId\": \"0\", \"error\": \"Ooops\" }\x1e"
        };

        call_number = std::min(call_number + 1, 1);

        if (call_number > 0)
        {
            callback_registered_event->wait();
        }

        callback(responses[call_number], nullptr);
    });

    auto hub_connection = create_hub_connection(websocket_client);

    auto mre = manual_reset_event<void>();
    hub_connection->start([&mre](std::exception_ptr exception)
    {
        mre.set(exception);
    });

    mre.get();

    hub_connection->invoke("method", signalr::value(signalr::value_type::array), [&mre](const signalr::value&, std::exception_ptr exception)
    {
        mre.set(exception);
    });

    callback_registered_event->cancel();

    try
    {
        mre.get();

        ASSERT_TRUE(false); // exception expected but not thrown
    }
    catch (const hub_exception& e)
    {
        ASSERT_STREQ("Ooops", e.what());
    }
}

TEST(invoke, unblocks_task_when_server_completes_call)
{
    auto callback_registered_event = std::make_shared<cancellation_token>();

    int call_number = -1;
    auto websocket_client = create_test_websocket_client(
        /* receive function */ [call_number, callback_registered_event](std::function<void(std::string, std::exception_ptr)> callback)
        mutable {
        std::string responses[]
        {
            "{ }\x1e",
            "{ \"type\": 3, \"invocationId\": \"0\" }\x1e"
        };

        call_number = std::min(call_number + 1, 1);

        if (call_number > 0)
        {
            callback_registered_event->wait();
        }

        callback(responses[call_number], nullptr);
    });

    auto hub_connection = create_hub_connection(websocket_client);

    auto mre = manual_reset_event<void>();
    hub_connection->start([&mre](std::exception_ptr exception)
    {
        mre.set(exception);
    });

    mre.get();

    hub_connection->invoke("method", signalr::value(signalr::value_type::array), [&mre](const signalr::value&, std::exception_ptr exception)
    {
        mre.set(exception);
    });

    callback_registered_event->cancel();

    mre.get();
}

TEST(invoke, passing_non_array_arguments_fails)
{
    auto callback_registered_event = std::make_shared<cancellation_token>();

    int call_number = -1;
    auto websocket_client = create_test_websocket_client(
        /* receive function */ [call_number, callback_registered_event](std::function<void(std::string, std::exception_ptr)> callback)
        mutable {
            std::string responses[]
            {
                "{ }\x1e",
                "{ \"type\": 3, \"invocationId\": \"0\" }\x1e"
            };

            call_number = std::min(call_number + 1, 1);

            if (call_number > 0)
            {
                callback_registered_event->wait();
            }

            callback(responses[call_number], nullptr);
        });

    auto hub_connection = create_hub_connection(websocket_client);

    auto mre = manual_reset_event<void>();
    hub_connection->start([&mre](std::exception_ptr exception)
        {
            mre.set(exception);
        });

    mre.get();

    hub_connection->invoke("method", signalr::value(true), [&mre](const signalr::value&, std::exception_ptr exception)
        {
            mre.set(exception);
        });

    try
    {
        mre.get();
        ASSERT_TRUE(false);
    }
    catch (const std::exception & ex)
    {
        ASSERT_STREQ("arguments should be an array", ex.what());
    }

    callback_registered_event->cancel();
}

TEST(receive, logs_if_callback_for_given_id_not_found)
{
    auto message_received_event = std::make_shared<cancellation_token>();
    auto handshake_sent = std::make_shared<cancellation_token>();

    int call_number = -1;
    auto websocket_client = create_test_websocket_client(
        /* receive function */ [call_number, message_received_event, handshake_sent](std::function<void(std::string, std::exception_ptr)> callback)
        mutable {
        std::string responses[]
        {
            "{ }\x1e",
            "{ \"type\": 3, \"invocationId\": \"0\" }\x1e",
            "{\"type\":6}\x1e"
        };

        handshake_sent->wait();

        call_number = std::min(call_number + 1, 2);

        if (call_number > 1)
        {
            message_received_event->cancel();
        }

        callback(responses[call_number], nullptr);
    },
    [handshake_sent](const std::string&, std::function<void(std::exception_ptr)> callback)
    {
        handshake_sent->cancel();
        callback(nullptr);
    });

    std::shared_ptr<log_writer> writer(std::make_shared<memory_log_writer>());
    auto hub_connection = create_hub_connection(websocket_client, writer, trace_level::info);

    auto mre = manual_reset_event<void>();
    hub_connection->start([&mre](std::exception_ptr exception)
    {
        mre.set(exception);
    });

    mre.get();

    ASSERT_FALSE(message_received_event->wait(5000));

    auto log_entries = std::dynamic_pointer_cast<memory_log_writer>(writer)->get_log_entries();
    ASSERT_TRUE(log_entries.size() > 1);

    auto entry = remove_date_from_log_entry(log_entries[2]);
    ASSERT_EQ("[info        ] no callback found for id: 0\n", entry) << dump_vector(log_entries);
}

TEST(invoke_void, invoke_creates_runtime_error)
{
   auto callback_registered_event = std::make_shared<cancellation_token>();

   int call_number = -1;
   auto websocket_client = create_test_websocket_client(
       /* receive function */ [call_number, callback_registered_event](std::function<void(std::string, std::exception_ptr)> callback)
       mutable {
       std::string responses[]
       {
           "{ }\x1e",
           "{ \"type\": 3, \"invocationId\": \"0\", \"error\": \"Ooops\" }\x1e"
       };

       call_number = std::min(call_number + 1, 1);

       if (call_number > 0)
       {
           callback_registered_event->wait();
       }

       callback(responses[call_number], nullptr);
   });

   auto hub_connection = create_hub_connection(websocket_client);

   auto mre = manual_reset_event<void>();
   hub_connection->start([&mre](std::exception_ptr exception)
   {
       mre.set(exception);
   });

   mre.get();

   hub_connection->invoke("method", signalr::value(signalr::value_type::array), [&mre](const signalr::value&, std::exception_ptr exception)
   {
       mre.set(exception);
   });

   callback_registered_event->cancel();

   try
   {
       mre.get();

       ASSERT_TRUE(false); // exception expected but not thrown
   }
   catch (const hub_exception & e)
   {
       ASSERT_STREQ("Ooops", e.what());
       ASSERT_FALSE(callback_registered_event->wait(0));
   }
}

TEST(connection_id, can_get_connection_id)
{
    int number = 0;
    auto websocket_client = create_test_websocket_client(
        /* receive function */ [number](std::function<void(std::string, std::exception_ptr)> callback) mutable { number = default_receive_func(number, callback); });
    auto hub_connection = create_hub_connection(websocket_client);

    ASSERT_EQ("", hub_connection->get_connection_id());

    auto mre = manual_reset_event<void>();
    hub_connection->start([&mre](std::exception_ptr exception)
    {
        mre.set(exception);
    });

    mre.get();
    auto connection_id = hub_connection->get_connection_id();

    hub_connection->stop([&mre](std::exception_ptr exception)
    {
        mre.set(exception);
    });

    mre.get();

    ASSERT_EQ("f7707523-307d-4cba-9abf-3eef701241e8", connection_id);
    ASSERT_EQ("f7707523-307d-4cba-9abf-3eef701241e8", hub_connection->get_connection_id());
}

TEST(on, event_name_must_not_be_empty_string)
{
    auto hub_connection = create_hub_connection();
    try
    {
        hub_connection->on("", [](const signalr::value&) {});

        ASSERT_TRUE(false); // exception expected but not thrown
    }
    catch (const std::invalid_argument& e)
    {
        ASSERT_STREQ("event_name cannot be empty", e.what());
    }
}

TEST(on, cannot_register_multiple_handlers_for_event)
{
    auto hub_connection = create_hub_connection();
    hub_connection->on("ping", [](const signalr::value&) {});

    try
    {
        hub_connection->on("ping", [](const signalr::value&) {});
        ASSERT_TRUE(false); // exception expected but not thrown
    }
    catch (const signalr_exception& e)
    {
        ASSERT_STREQ("an action for this event has already been registered. event name: ping", e.what());
    }
}

TEST(on, cannot_register_handler_if_connection_not_in_disconnected_state)
{
    try
    {
        int number = 0;
        auto websocket_client = create_test_websocket_client(
            /* receive function */ [number](std::function<void(std::string, std::exception_ptr)> callback) mutable { number = default_receive_func(number, callback); });
        auto hub_connection = create_hub_connection(websocket_client);

        auto mre = manual_reset_event<void>();
        hub_connection->start([&mre](std::exception_ptr exception)
        {
            mre.set(exception);
        });

        mre.get();

        hub_connection->on("myfunc", [](const signalr::value&) {});

        ASSERT_TRUE(false); // exception expected but not thrown
    }
    catch (const signalr_exception& e)
    {
        ASSERT_STREQ("can't register a handler if the connection is in a disconnected state", e.what());
    }
}

TEST(invoke, invoke_throws_when_the_underlying_connection_is_not_valid)
{
    auto hub_connection = create_hub_connection();

    auto mre = manual_reset_event<void>();
    hub_connection->invoke("method", signalr::value(signalr::value_type::array), [&mre](const signalr::value&, std::exception_ptr exception)
    {
        mre.set(exception);
    });

    try
    {
        mre.get();
        ASSERT_TRUE(false); // exception expected but not thrown
    }
    catch (const signalr_exception& e)
    {
        ASSERT_STREQ("cannot send data when the connection is not in the connected state. current connection state: disconnected", e.what());
    }
}

TEST(invoke, send_throws_when_the_underlying_connection_is_not_valid)
{
    auto hub_connection = create_hub_connection();

    auto mre = manual_reset_event<void>();
    hub_connection->invoke("method", signalr::value(signalr::value_type::array), [&mre](const signalr::value&, std::exception_ptr exception)
    {
        mre.set(exception);
    });

    try
    {
        mre.get();
        ASSERT_TRUE(false); // exception expected but not thrown
    }
    catch (const signalr_exception& e)
    {
        ASSERT_STREQ("cannot send data when the connection is not in the connected state. current connection state: disconnected", e.what());
    }
}
