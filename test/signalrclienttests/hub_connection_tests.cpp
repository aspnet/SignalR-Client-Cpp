// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#include "stdafx.h"
#include "test_utils.h"
#include "test_http_client.h"
#include "signalrclient/hub_connection_builder.h"
#include "memory_log_writer.h"
#include "signalrclient/hub_exception.h"
#include "signalrclient/signalr_exception.h"
#include "test_websocket_client.h"

using namespace signalr;

std::shared_ptr<hub_connection> create_hub_connection(std::shared_ptr<websocket_client> websocket_client = create_test_websocket_client(),
    std::shared_ptr<log_writer> log_writer = std::make_shared<memory_log_writer>(), trace_level trace_level = trace_level::all)
{
    return hub_connection_builder::create(create_uri())
        .with_logging(log_writer, trace_level)
        .with_http_client(create_test_http_client())
        .with_websocket_factory([websocket_client](const signalr_client_config&) { return websocket_client; })
        .build();
}

template<typename TValue>
std::string build_websocket_message_string(TValue value)
{
    std::ostringstream stream;
    stream << "{ \"type\": 3, \"invocationId\": \"0\", \"result\": [" << value << "] }\x1e";

    return stream.str();
}

template<int32_t precision>
std::string format_double_as_string(double value)
{
    std::ostringstream stream;
    stream << std::setprecision(precision) << value;

    return stream.str();
}

template<typename TValue>
void invoke_common_numeric_handling_logic(TValue original_value, std::function<void(TValue, TValue)> assertion_callback)
{
    std::string websocket_message = build_websocket_message_string(original_value);

    auto websocket_client = create_test_websocket_client();
    auto hub_connection = create_hub_connection(websocket_client);

    auto connect_mre = manual_reset_event<void>();

    hub_connection->start([&connect_mre](std::exception_ptr exception)
        {
            connect_mre.set(exception);
        });

    ASSERT_FALSE(websocket_client->receive_loop_started.wait(5000));
    ASSERT_FALSE(websocket_client->handshake_sent.wait(5000));

    websocket_client->receive_message("{ }\x1e");

    connect_mre.get();

    auto callback_mre = manual_reset_event<signalr::value>();

    std::vector<signalr::value> arr{ signalr::value((double)original_value) };

    hub_connection->invoke("method", signalr::value(arr), [&callback_mre](const signalr::value& message, std::exception_ptr exception)
        {
            if (exception)
            {
                callback_mre.set(exception);
            }
            else
            {
                callback_mre.set(message);
            }
        });

    websocket_client->receive_message(websocket_message);

    auto result = callback_mre.get();

    ASSERT_TRUE(result.is_array());

    auto array = result.as_array();

    ASSERT_EQ(1, array.size());

    auto value = static_cast<TValue>(array[0].as_double());

    assertion_callback(value, original_value);
}

TEST(url, negotiate_appended_to_url)
{
    std::string base_urls[] = { "http://fakeuri", "http://fakeuri/" };

    for (const auto& base_url : base_urls)
    {
        std::string requested_url;
        auto http_client = std::make_shared<test_http_client>([&requested_url](const std::string& url, http_request)
        {
            requested_url = url;
            return http_response{ 404, "" };
        });

        auto hub_connection = hub_connection_builder::create(base_url)
            .with_logging(std::make_shared<memory_log_writer>(), trace_level::none)
            .with_http_client(http_client)
            .with_websocket_factory([](const signalr_client_config&) { return create_test_websocket_client(); })
            .build();

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
    auto websocket_client = create_test_websocket_client();
    auto hub_connection = create_hub_connection(websocket_client);

    auto mre = manual_reset_event<void>();
    hub_connection->start([&mre](std::exception_ptr exception)
    {
        mre.set(exception);
    });

    ASSERT_FALSE(websocket_client->receive_loop_started.wait(5000));
    ASSERT_FALSE(websocket_client->handshake_sent.wait(5000));
    websocket_client->receive_message("{}\x1e");

    mre.get();

    ASSERT_EQ(connection_state::connected, hub_connection->get_connection_state());
}

TEST(start, start_sends_handshake)
{
    auto message = std::make_shared<std::string>();
    auto websocket_client = create_test_websocket_client(
        /* send function */ [message](const std::string& msg, std::function<void(std::exception_ptr)> callback) { *message = msg; callback(nullptr); });
    auto hub_connection = create_hub_connection(websocket_client);

    auto mre = manual_reset_event<void>();
    hub_connection->start([&mre](std::exception_ptr exception)
    {
        mre.set(exception);
    });

    ASSERT_FALSE(websocket_client->receive_loop_started.wait(5000));
    ASSERT_FALSE(websocket_client->handshake_sent.wait(5000));
    websocket_client->receive_message("{}\x1e");

    mre.get();

    ASSERT_EQ("{\"protocol\":\"json\",\"version\":1}\x1e", *message);

    ASSERT_EQ(connection_state::connected, hub_connection->get_connection_state());
}

TEST(start, start_waits_for_handshake_response)
{
    auto websocket_client = create_test_websocket_client();
    auto hub_connection = create_hub_connection(websocket_client);

    auto mre = manual_reset_event<void>();
    auto done = false;
    hub_connection->start([&mre, &done](std::exception_ptr exception)
    {
        done = true;
        mre.set(exception);
    });

    ASSERT_FALSE(websocket_client->receive_loop_started.wait(5000));
    ASSERT_FALSE(websocket_client->handshake_sent.wait(5000));
    ASSERT_FALSE(done);
    websocket_client->receive_message("{}\x1e");
    mre.get();

    ASSERT_EQ(connection_state::connected, hub_connection->get_connection_state());
}

TEST(start, start_fails_for_handshake_response_with_error)
{
    auto websocket_client = create_test_websocket_client();
    auto hub_connection = create_hub_connection(websocket_client);

    auto mre = manual_reset_event<void>();
    hub_connection->start([&mre](std::exception_ptr exception)
    {
        mre.set(exception);
    });

    ASSERT_FALSE(websocket_client->receive_loop_started.wait(5000));
    ASSERT_FALSE(websocket_client->handshake_sent.wait(5000));
    websocket_client->receive_message("{\"error\":\"bad things\"}\x1e");

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

TEST(start, start_fails_for_incomplete_handshake_response)
{
    auto websocket_client = create_test_websocket_client();
    auto hub_connection = create_hub_connection(websocket_client);

    auto mre = manual_reset_event<void>();
    hub_connection->start([&mre](std::exception_ptr exception)
        {
            mre.set(exception);
        });

    ASSERT_FALSE(websocket_client->receive_loop_started.wait(5000));
    ASSERT_FALSE(websocket_client->handshake_sent.wait(5000));
    websocket_client->receive_message("{}");

    try
    {
        mre.get();
        ASSERT_TRUE(false);
    }
    catch (const std::exception & ex)
    {
        ASSERT_STREQ("connection closed while handshake was in progress.", ex.what());
    }

    ASSERT_EQ(connection_state::disconnected, hub_connection->get_connection_state());
}

TEST(start, start_fails_for_invalid_json_handshake_response)
{
    auto websocket_client = create_test_websocket_client();
    auto hub_connection = create_hub_connection(websocket_client);

    auto mre = manual_reset_event<void>();
    hub_connection->start([&mre](std::exception_ptr exception)
        {
            mre.set(exception);
        });

    ASSERT_FALSE(websocket_client->receive_loop_started.wait(5000));
    ASSERT_FALSE(websocket_client->handshake_sent.wait(5000));
    websocket_client->receive_message("{\"name\"}\x1e");

    try
    {
        mre.get();
        ASSERT_TRUE(false);
    }
    catch (const std::exception & ex)
    {
        ASSERT_STREQ("connection closed while handshake was in progress.", ex.what());
    }

    ASSERT_EQ(connection_state::disconnected, hub_connection->get_connection_state());
}

TEST(start, start_fails_if_stop_called_before_handshake_response)
{
    auto websocket_client = create_test_websocket_client();
    auto hub_connection = create_hub_connection(websocket_client);

    auto mre = manual_reset_event<void>();
    hub_connection->start([&mre](std::exception_ptr exception)
    {
        mre.set(exception);
    });

    ASSERT_FALSE(websocket_client->receive_loop_started.wait(5000));
    ASSERT_FALSE(websocket_client->handshake_sent.wait(5000));

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
}

TEST(stop, stop_stops_connection)
{
    auto websocket_client = create_test_websocket_client();
    auto hub_connection = create_hub_connection(websocket_client);

    auto mre = manual_reset_event<void>();
    hub_connection->start([&mre](std::exception_ptr exception)
    {
        mre.set(exception);
    });

    ASSERT_FALSE(websocket_client->receive_loop_started.wait(5000));
    ASSERT_FALSE(websocket_client->handshake_sent.wait(5000));
    websocket_client->receive_message("{}\x1e");

    mre.get();

    hub_connection->stop([&mre](std::exception_ptr exception)
    {
        mre.set(exception);
    });

    mre.get();

    ASSERT_EQ(connection_state::disconnected, hub_connection->get_connection_state());
}

TEST(stop, does_nothing_on_disconnected_connection)
{
    auto websocket_client = create_test_websocket_client();
    auto hub_connection = create_hub_connection(websocket_client);

    auto mre = manual_reset_event<void>();

    ASSERT_EQ(connection_state::disconnected, hub_connection->get_connection_state());

    hub_connection->stop([&mre](std::exception_ptr exception)
        {
            mre.set(exception);
        });

    mre.get();

    ASSERT_EQ(connection_state::disconnected, hub_connection->get_connection_state());
}

TEST(stop, second_stop_waits_for_first_stop)
{
    auto transport_stop_mre = manual_reset_event<void>();
    auto websocket_client = create_test_websocket_client();
    websocket_client->set_close_function([&transport_stop_mre](std::function<void (std::exception_ptr)> callback)
        {
            transport_stop_mre.get();
            callback(nullptr);
        });
    auto hub_connection = create_hub_connection(websocket_client);
    bool connection_closed = false;
    hub_connection->set_disconnected([&connection_closed]()
        {
            connection_closed = true;
        });

    auto mre = manual_reset_event<void>();
    hub_connection->start([&mre](std::exception_ptr exception)
        {
            mre.set(exception);
        });

    ASSERT_FALSE(websocket_client->receive_loop_started.wait(5000));
    ASSERT_FALSE(websocket_client->handshake_sent.wait(5000));
    websocket_client->receive_message("{}\x1e");

    mre.get();

    hub_connection->stop([&mre](std::exception_ptr exception)
        {
            mre.set(exception);
        });

    bool second_stop_called = false;
    auto second_mre = manual_reset_event<void>();
    hub_connection->stop([&second_stop_called, &second_mre](std::exception_ptr exception)
        {
            second_stop_called = true;
            second_mre.set(exception);
        });

    ASSERT_FALSE(second_stop_called);
    ASSERT_FALSE(connection_closed);

    transport_stop_mre.set();
    mre.get();

    ASSERT_TRUE(connection_closed);
    second_mre.get();
    ASSERT_TRUE(second_stop_called);

    ASSERT_EQ(connection_state::disconnected, hub_connection->get_connection_state());
}

TEST(stop, blocking_stop_callback_does_not_prevent_start)
{
    auto transport_stop_mre = manual_reset_event<void>();
    auto websocket_client = create_test_websocket_client();
    auto hub_connection = create_hub_connection(websocket_client);

    auto mre = manual_reset_event<void>();
    hub_connection->start([&mre](std::exception_ptr exception)
        {
            mre.set(exception);
        });

    ASSERT_FALSE(websocket_client->receive_loop_started.wait(5000));
    ASSERT_FALSE(websocket_client->handshake_sent.wait(5000));
    websocket_client->receive_message("{}\x1e");

    mre.get();

    auto blocking_mre = manual_reset_event<void>();
    hub_connection->stop([&mre, &blocking_mre](std::exception_ptr exception)
        {
            mre.set(exception);
            blocking_mre.get();
            mre.set();
        });

    mre.get();
    ASSERT_EQ(connection_state::disconnected, hub_connection->get_connection_state());

    hub_connection->start([&mre](std::exception_ptr exception)
        {
            mre.set(exception);
        });

    ASSERT_FALSE(websocket_client->receive_loop_started.wait(5000));
    ASSERT_FALSE(websocket_client->handshake_sent.wait(5000));
    websocket_client->receive_message("{}\x1e");

    mre.get();
    ASSERT_EQ(connection_state::connected, hub_connection->get_connection_state());

    blocking_mre.set();
    mre.get();
}

TEST(stop, disconnected_callback_called_when_hub_connection_stops)
{
    auto websocket_client = create_test_websocket_client();
    auto hub_connection = create_hub_connection(websocket_client);

    auto disconnected_invoked = false;
    hub_connection->set_disconnected([&disconnected_invoked]() { disconnected_invoked = true; });

    auto mre = manual_reset_event<void>();
    hub_connection->start([&mre](std::exception_ptr exception)
    {
        mre.set(exception);
    });

    ASSERT_FALSE(websocket_client->receive_loop_started.wait(5000));
    ASSERT_FALSE(websocket_client->handshake_sent.wait(5000));
    websocket_client->receive_message("{}\x1e");

    mre.get();

    hub_connection->stop([&mre](std::exception_ptr exception)
    {
        mre.set(exception);
    });

    mre.get();

    ASSERT_TRUE(disconnected_invoked);
    ASSERT_EQ(connection_state::disconnected, hub_connection->get_connection_state());
}

TEST(stop, disconnected_callback_called_when_transport_error_occurs)
{
    auto websocket_client = create_test_websocket_client();
    auto hub_connection = create_hub_connection(websocket_client);

    auto disconnected_invoked = manual_reset_event<void>();
    hub_connection->set_disconnected([&disconnected_invoked]() { disconnected_invoked.set(); });

    auto mre = manual_reset_event<void>();
    hub_connection->start([&mre](std::exception_ptr exception)
        {
            mre.set(exception);
        });

    ASSERT_FALSE(websocket_client->receive_loop_started.wait(5000));
    ASSERT_FALSE(websocket_client->handshake_sent.wait(5000));
    websocket_client->receive_message("{}\x1e");

    mre.get();

    websocket_client->receive_message(std::make_exception_ptr(std::runtime_error("transport error")));

    disconnected_invoked.get();
    ASSERT_EQ(connection_state::disconnected, hub_connection->get_connection_state());
}

TEST(stop, connection_stopped_when_going_out_of_scope)
{
    std::shared_ptr<log_writer> writer(std::make_shared<memory_log_writer>());

    {
        auto websocket_client = create_test_websocket_client();
        auto hub_connection = create_hub_connection(websocket_client, writer, trace_level::state_changes);

        auto mre = manual_reset_event<void>();
        hub_connection->start([&mre](std::exception_ptr exception)
        {
            mre.set(exception);
        });

        ASSERT_FALSE(websocket_client->receive_loop_started.wait(5000));
        ASSERT_FALSE(websocket_client->handshake_sent.wait(5000));
        websocket_client->receive_message("{}\x1e");

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
    auto websocket_client = create_test_websocket_client();

    auto hub_connection = create_hub_connection(websocket_client);
    auto mre = manual_reset_event<void>();
    hub_connection->start([&mre](std::exception_ptr exception)
    {
        mre.set(exception);
    });

    ASSERT_FALSE(websocket_client->receive_loop_started.wait(5000));
    ASSERT_FALSE(websocket_client->handshake_sent.wait(5000));
    websocket_client->receive_message("{}\x1e");

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
    auto websocket_client = create_test_websocket_client();

    auto invoke_mre = manual_reset_event<void>();

    {
        auto hub_connection = create_hub_connection(websocket_client);
        auto mre = manual_reset_event<void>();
        hub_connection->start([&mre](std::exception_ptr exception)
        {
            mre.set(exception);
        });

        ASSERT_FALSE(websocket_client->receive_loop_started.wait(5000));
        ASSERT_FALSE(websocket_client->handshake_sent.wait(5000));
        websocket_client->receive_message("{}\x1e");

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
    auto websocket_client = create_test_websocket_client();

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

    ASSERT_FALSE(websocket_client->receive_loop_started.wait(5000));
    ASSERT_FALSE(websocket_client->handshake_sent.wait(5000));
    websocket_client->receive_message("{}\x1e");
    websocket_client->receive_message("{ \"type\": 1, \"target\": \"BROADcast\", \"arguments\": [ \"message\", 1 ] }\x1e");

    mre.get();
    ASSERT_FALSE(on_broadcast_event->wait(5000));

    auto array = payload->as_array();
    ASSERT_EQ(2, array.size());
    ASSERT_EQ("message", array[0].as_string());
    ASSERT_EQ(1, array[1].as_double());
}

TEST(hub_invocation, hub_connection_can_receive_handshake_and_message_in_same_payload)
{
    auto websocket_client = create_test_websocket_client();

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

    ASSERT_FALSE(websocket_client->receive_loop_started.wait(5000));
    ASSERT_FALSE(websocket_client->handshake_sent.wait(5000));
    websocket_client->receive_message("{ }\x1e{ \"type\": 1, \"target\": \"BROADcast\", \"arguments\": [ \"message\", 1 ] }\x1e");

    mre.get();
    ASSERT_FALSE(on_broadcast_event->wait(5000));

    auto array = payload->as_array();
    ASSERT_EQ(2, array.size());
    ASSERT_EQ("message", array[0].as_string());
    ASSERT_EQ(1, array[1].as_double());
}

TEST(hub_invocation, hub_connection_can_receive_multiple_messages_in_same_payload)
{
    auto websocket_client = create_test_websocket_client();

    auto hub_connection = create_hub_connection(websocket_client);

    auto payload = std::make_shared<signalr::value>();
    int count = 0;
    auto on_broadcast_event = std::make_shared<cancellation_token>();
    hub_connection->on("broadCAST", [&count, on_broadcast_event, payload](const signalr::value& message)
        {
            ++count;
            *payload = message;
            if (count == 2)
            {
                on_broadcast_event->cancel();
            }
        });

    auto mre = manual_reset_event<void>();
    hub_connection->start([&mre](std::exception_ptr exception)
        {
            mre.set(exception);
        });

    ASSERT_FALSE(websocket_client->receive_loop_started.wait(5000));
    ASSERT_FALSE(websocket_client->handshake_sent.wait(5000));
    websocket_client->receive_message("{ }\x1e");
    websocket_client->receive_message("{ \"type\": 1, \"target\": \"BROADcast\", \"arguments\": [ \"message\", 1 ] }\x1e{ \"type\": 1, \"target\": \"BROADcast\", \"arguments\": [ \"message\", 1 ] }\x1e");

    mre.get();
    ASSERT_FALSE(on_broadcast_event->wait(5000));

    auto array = payload->as_array();
    ASSERT_EQ(2, array.size());
    ASSERT_EQ("message", array[0].as_string());
    ASSERT_EQ(1, array[1].as_double());

    ASSERT_EQ(2, count);
}

TEST(hub_invocation, hub_connection_closes_when_invocation_response_missing_arguments)
{
    auto websocket_client = create_test_websocket_client();

    std::shared_ptr<log_writer> writer(std::make_shared<memory_log_writer>());
    auto hub_connection = create_hub_connection(websocket_client, writer, trace_level::errors);

    auto on_closed_event = std::make_shared<cancellation_token>();
    hub_connection->set_disconnected([on_closed_event]()
        {
            on_closed_event->cancel();
        });

    auto mre = manual_reset_event<void>();
    hub_connection->start([&mre](std::exception_ptr exception)
        {
            mre.set(exception);
        });

    ASSERT_FALSE(websocket_client->receive_loop_started.wait(5000));
    ASSERT_FALSE(websocket_client->handshake_sent.wait(5000));
    websocket_client->receive_message("{ }\x1e");

    mre.get();

    websocket_client->receive_message("{ \"type\": 1, \"target\": \"broadcast\" }\x1e");
    ASSERT_FALSE(on_closed_event->wait(5000));

    auto log_entries = std::dynamic_pointer_cast<memory_log_writer>(writer)->get_log_entries();
    ASSERT_TRUE(log_entries.size() == 1);

    auto entry = remove_date_from_log_entry(log_entries[0]);
    ASSERT_EQ("[error       ] error occured when parsing response: Field 'arguments' not found for 'invocation' message. response: { \"type\": 1, \"target\": \"broadcast\" }\x1e\n", entry) << dump_vector(log_entries);
}

TEST(hub_invocation, hub_connection_closes_when_invocation_response_missing_target)
{
    auto websocket_client = create_test_websocket_client();

    std::shared_ptr<log_writer> writer(std::make_shared<memory_log_writer>());
    auto hub_connection = create_hub_connection(websocket_client, writer, trace_level::errors);

    auto on_closed_event = std::make_shared<cancellation_token>();
    hub_connection->set_disconnected([on_closed_event]()
        {
            on_closed_event->cancel();
        });

    auto mre = manual_reset_event<void>();
    hub_connection->start([&mre](std::exception_ptr exception)
        {
            mre.set(exception);
        });

    ASSERT_FALSE(websocket_client->receive_loop_started.wait(5000));
    ASSERT_FALSE(websocket_client->handshake_sent.wait(5000));
    websocket_client->receive_message("{ }\x1e");

    mre.get();

    websocket_client->receive_message("{ \"type\": 1, \"arguments\": [] }\x1e");
    ASSERT_FALSE(on_closed_event->wait(5000));

    auto log_entries = std::dynamic_pointer_cast<memory_log_writer>(writer)->get_log_entries();
    ASSERT_TRUE(log_entries.size() == 1);

    auto entry = remove_date_from_log_entry(log_entries[0]);
    ASSERT_EQ("[error       ] error occured when parsing response: Field 'target' not found for 'invocation' message. response: { \"type\": 1, \"arguments\": [] }\x1e\n", entry) << dump_vector(log_entries);
}

TEST(send, creates_correct_payload)
{
    std::string payload;
    bool handshakeReceived = false;

    auto websocket_client = create_test_websocket_client(
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

    ASSERT_FALSE(websocket_client->receive_loop_started.wait(5000));
    ASSERT_FALSE(websocket_client->handshake_sent.wait(5000));
    websocket_client->receive_message("{ }\x1e");

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
    auto websocket_client = create_test_websocket_client();

    auto hub_connection = create_hub_connection(websocket_client);

    auto mre = manual_reset_event<void>();
    hub_connection->start([&mre](std::exception_ptr exception)
    {
        mre.set(exception);
    });

    ASSERT_FALSE(websocket_client->receive_loop_started.wait(5000));
    ASSERT_FALSE(websocket_client->handshake_sent.wait(5000));
    websocket_client->receive_message("{ }\x1e");

    mre.get();

    // wont block waiting for server response
    hub_connection->send("method", signalr::value(signalr::value_type::array), [&mre](std::exception_ptr exception)
    {
        mre.set(exception);
    });
    mre.get();
}

TEST(send, passing_non_array_arguments_fails)
{
    auto websocket_client = create_test_websocket_client();

    auto hub_connection = create_hub_connection(websocket_client);

    auto mre = manual_reset_event<void>();
    hub_connection->start([&mre](std::exception_ptr exception)
        {
            mre.set(exception);
        });

    ASSERT_FALSE(websocket_client->receive_loop_started.wait(5000));
    ASSERT_FALSE(websocket_client->handshake_sent.wait(5000));
    websocket_client->receive_message("{ }\x1e");

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
}

TEST(invoke, creates_correct_payload)
{
    std::string payload;
    bool handshakeReceived = false;

    auto websocket_client = create_test_websocket_client(
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

    ASSERT_FALSE(websocket_client->receive_loop_started.wait(5000));
    ASSERT_FALSE(websocket_client->handshake_sent.wait(5000));
    websocket_client->receive_message("{ }\x1e");

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
    auto websocket_client = create_test_websocket_client(
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

    ASSERT_FALSE(websocket_client->receive_loop_started.wait(5000));
    ASSERT_FALSE(websocket_client->handshake_sent.wait(5000));
    websocket_client->receive_message("{ }\x1e");

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

    // stop completes all outstanding callbacks so if we did not remove a callback when `invoke` failed an
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
    auto websocket_client = create_test_websocket_client();

    auto hub_connection = create_hub_connection(websocket_client);

    auto mre = manual_reset_event<void>();
    hub_connection->start([&mre](std::exception_ptr exception)
    {
        mre.set(exception);
    });

    ASSERT_FALSE(websocket_client->receive_loop_started.wait(5000));
    ASSERT_FALSE(websocket_client->handshake_sent.wait(5000));
    websocket_client->receive_message("{ }\x1e");

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

    websocket_client->receive_message("{ \"type\": 3, \"invocationId\": \"0\", \"result\": \"abc\" }\x1e");

    auto result = invoke_mre.get();

    ASSERT_TRUE(result.is_string());
    ASSERT_EQ("abc", result.as_string());
}

TEST(invoke, invoke_propagates_errors_from_server_as_hub_exceptions)
{
    auto websocket_client = create_test_websocket_client();

    auto hub_connection = create_hub_connection(websocket_client);

    auto mre = manual_reset_event<void>();
    hub_connection->start([&mre](std::exception_ptr exception)
    {
        mre.set(exception);
    });

    ASSERT_FALSE(websocket_client->receive_loop_started.wait(5000));
    ASSERT_FALSE(websocket_client->handshake_sent.wait(5000));
    websocket_client->receive_message("{ }\x1e");

    mre.get();

    hub_connection->invoke("method", signalr::value(signalr::value_type::array), [&mre](const signalr::value&, std::exception_ptr exception)
    {
        mre.set(exception);
    });

    websocket_client->receive_message("{ \"type\": 3, \"invocationId\": \"0\", \"error\": \"Ooops\" }\x1e");

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
    auto websocket_client = create_test_websocket_client();

    auto hub_connection = create_hub_connection(websocket_client);

    auto mre = manual_reset_event<void>();
    hub_connection->start([&mre](std::exception_ptr exception)
    {
        mre.set(exception);
    });

    ASSERT_FALSE(websocket_client->receive_loop_started.wait(5000));
    ASSERT_FALSE(websocket_client->handshake_sent.wait(5000));
    websocket_client->receive_message("{ }\x1e");

    mre.get();

    hub_connection->invoke("method", signalr::value(signalr::value_type::array), [&mre](const signalr::value&, std::exception_ptr exception)
    {
        mre.set(exception);
    });

    websocket_client->receive_message("{ \"type\": 3, \"invocationId\": \"0\" }\x1e");

    mre.get();
}

TEST(invoke, passing_non_array_arguments_fails)
{
    auto websocket_client = create_test_websocket_client();

    auto hub_connection = create_hub_connection(websocket_client);

    auto mre = manual_reset_event<void>();
    hub_connection->start([&mre](std::exception_ptr exception)
        {
            mre.set(exception);
        });

    ASSERT_FALSE(websocket_client->receive_loop_started.wait(5000));
    ASSERT_FALSE(websocket_client->handshake_sent.wait(5000));
    websocket_client->receive_message("{ }\x1e");

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
}

TEST(receive, logs_if_callback_for_given_id_not_found)
{
    auto websocket_client = create_test_websocket_client();

    std::shared_ptr<log_writer> writer(std::make_shared<memory_log_writer>());
    auto hub_connection = create_hub_connection(websocket_client, writer, trace_level::info);

    auto mre = manual_reset_event<void>();
    hub_connection->start([&mre](std::exception_ptr exception)
    {
        mre.set(exception);
    });

    ASSERT_FALSE(websocket_client->receive_loop_started.wait(5000));
    ASSERT_FALSE(websocket_client->handshake_sent.wait(5000));
    websocket_client->receive_message("{ }\x1e");

    mre.get();

    websocket_client->receive_message("{ \"type\": 3, \"invocationId\": \"0\" }\x1e");

    // hack? blocks until previous message has been processed
    websocket_client->receive_message("{ \"type\": 6 }\x1e");

    auto log_entries = std::dynamic_pointer_cast<memory_log_writer>(writer)->get_log_entries();
    ASSERT_TRUE(log_entries.size() > 2);

    auto entry = remove_date_from_log_entry(log_entries[2]);
    ASSERT_EQ("[info        ] no callback found for id: 0\n", entry) << dump_vector(log_entries);
}

TEST(receive, closes_if_error_from_parsing)
{
    auto websocket_client = create_test_websocket_client();

    std::shared_ptr<log_writer> writer(std::make_shared<memory_log_writer>());
    auto hub_connection = create_hub_connection(websocket_client, writer, trace_level::info);

    auto disconnect_mre = manual_reset_event<void>();
    hub_connection->set_disconnected([&disconnect_mre]()
        {
            disconnect_mre.set();
        });

    auto mre = manual_reset_event<void>();
    hub_connection->start([&mre](std::exception_ptr exception)
        {
            mre.set(exception);
        });

    ASSERT_FALSE(websocket_client->receive_loop_started.wait(5000));
    ASSERT_FALSE(websocket_client->handshake_sent.wait(5000));
    websocket_client->receive_message("{ }\x1e");

    mre.get();

    websocket_client->receive_message("{ \"type\": 3, \"invocationId\": \"0\": \"bad\" }\x1e");

    disconnect_mre.get();

    ASSERT_EQ(connection_state::disconnected, hub_connection->get_connection_state());
}

TEST(invoke_void, invoke_creates_runtime_error)
{
   auto websocket_client = create_test_websocket_client();

   auto hub_connection = create_hub_connection(websocket_client);

   auto mre = manual_reset_event<void>();
   hub_connection->start([&mre](std::exception_ptr exception)
   {
       mre.set(exception);
   });

   ASSERT_FALSE(websocket_client->receive_loop_started.wait(5000));
   ASSERT_FALSE(websocket_client->handshake_sent.wait(5000));
   websocket_client->receive_message("{ }\x1e");

   mre.get();

   hub_connection->invoke("method", signalr::value(signalr::value_type::array), [&mre](const signalr::value&, std::exception_ptr exception)
   {
       mre.set(exception);
   });

   websocket_client->receive_message("{ \"type\": 3, \"invocationId\": \"0\", \"error\": \"Ooops\" }\x1e");

   try
   {
       mre.get();

       ASSERT_TRUE(false); // exception expected but not thrown
   }
   catch (const hub_exception & e)
   {
       ASSERT_STREQ("Ooops", e.what());
   }
}

TEST(connection_id, can_get_connection_id)
{
    auto websocket_client = create_test_websocket_client();
    auto hub_connection = create_hub_connection(websocket_client);

    ASSERT_EQ("", hub_connection->get_connection_id());

    auto mre = manual_reset_event<void>();
    hub_connection->start([&mre](std::exception_ptr exception)
    {
        mre.set(exception);
    });

    ASSERT_FALSE(websocket_client->receive_loop_started.wait(5000));
    ASSERT_FALSE(websocket_client->handshake_sent.wait(5000));
    websocket_client->receive_message("{ }\x1e");

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
        auto websocket_client = create_test_websocket_client();
        auto hub_connection = create_hub_connection(websocket_client);

        auto mre = manual_reset_event<void>();
        hub_connection->start([&mre](std::exception_ptr exception)
        {
            mre.set(exception);
        });

        ASSERT_FALSE(websocket_client->receive_loop_started.wait(5000));
        ASSERT_FALSE(websocket_client->handshake_sent.wait(5000));
        websocket_client->receive_message("{ }\x1e");

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

TEST(invoke, invoke_handles_UINT32_MAX_value_as_unsigned_int)
{
    invoke_common_numeric_handling_logic<unsigned int>(UINT32_MAX, [](unsigned int result, unsigned int expected)
        {
            ASSERT_EQ(result, expected);
        });
}

TEST(invoke, invoke_handles_INT64_MIN_value_as_int64_t)
{
    invoke_common_numeric_handling_logic<long long>(INT64_MIN, [](long long result, long long expected)
        {
            ASSERT_EQ(result, expected);
        });
}

TEST(invoke, invoke_handles_INT64_MIN_minus_1_value_as_double)
{
    invoke_common_numeric_handling_logic<double>((INT64_MIN - 1.0), [](double result, double expected)
        {
            std::string result_string = format_double_as_string<6>(result);
            std::string expected_string = format_double_as_string<6>(expected);

            ASSERT_EQ(result_string, expected_string);
        });
}

TEST(invoke, invoke_handles_UINT64_MAX_value_as_uint64_t)
{
    invoke_common_numeric_handling_logic<long long>(UINT64_MAX, [](long long result, long long expected)
        {
            ASSERT_EQ(result, expected);
        });
}

TEST(invoke, invoke_handles_UINT64_MAX_plus_1_value_as_double)
{
    invoke_common_numeric_handling_logic<double>((UINT64_MAX + 1.0), [](double result, double expected)
        {
            std::string result_string = format_double_as_string<6>(result);
            std::string expected_string = format_double_as_string<6>(expected);

            ASSERT_EQ(result_string, expected_string);
        });
}

TEST(invoke, invoke_handles_non_integer_value_as_double)
{
    invoke_common_numeric_handling_logic<double>(3.1415926, [](double result, double expected)
        {
            std::string result_string = format_double_as_string<6>(result);
            std::string expected_string = format_double_as_string<6>(expected);

            ASSERT_EQ(result_string, expected_string);
        });
}

TEST(invoke, invoke_handles_zero_value_as_int)
{
    invoke_common_numeric_handling_logic<int>(0, [](int result, int expected)
        {
            ASSERT_EQ(result, expected);
        });
}
