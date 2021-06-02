// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#include "stdafx.h"
#include "test_utils.h"
#include "test_websocket_client.h"
#include "connection_impl.h"
#include "signalrclient/trace_level.h"
#include "memory_log_writer.h"
#include "signalr_default_scheduler.h"
#include "signalrclient/signalr_exception.h"
#include "signalrclient/web_exception.h"
#include "test_http_client.h"

using namespace signalr;

static std::shared_ptr<connection_impl> create_connection(std::shared_ptr<test_websocket_client> websocket_client = create_test_websocket_client(),
    std::shared_ptr<log_writer> log_writer = std::make_shared<memory_log_writer>(), trace_level trace_level = trace_level::verbose)
{
    return connection_impl::create(create_uri(), trace_level, log_writer, create_test_http_client(),
        [websocket_client](const signalr_client_config& config)
        {
            websocket_client->set_config(config);
            return websocket_client;
        });
}

TEST(connection_impl_connection_state, initial_connection_state_is_disconnected)
{
    auto connection = create_connection();

    ASSERT_EQ(connection_state::disconnected, connection->get_connection_state());
}

TEST(connection_impl_start, cannot_start_non_disconnected_exception)
{
    auto connection = create_connection();

    auto mre = manual_reset_event<void>();
    connection->start([&mre](std::exception_ptr exception)
        {
            mre.set(exception);
        });

    mre.get();

    try
    {
        connection->start([&mre](std::exception_ptr exception)
            {
                mre.set(exception);
            });
        mre.get();
        ASSERT_TRUE(false); // exception not thrown
    }
    catch (const signalr_exception & e)
    {
        ASSERT_STREQ("cannot start a connection that is not in the disconnected state", e.what());
    }
}

TEST(connection_impl_start, connection_state_is_connecting_when_connection_is_being_started)
{
    std::shared_ptr<log_writer> writer(std::make_shared<memory_log_writer>());

    auto state_mre = manual_reset_event<void>();
    auto websocket_client = create_test_websocket_client(
        /* send function */ [](const std::string&, std::function<void(std::exception_ptr)> callback) { callback(std::make_exception_ptr(std::runtime_error("should not be invoked"))); },
        /* connect function */[&state_mre](const std::string&, std::function<void(std::exception_ptr)> callback)
        {
            state_mre.get();
            callback(std::make_exception_ptr(std::runtime_error("connecting failed")));
        });

    auto connection = create_connection(websocket_client, writer, trace_level::error);

    auto mre = manual_reset_event<void>();
    connection->start([&mre](std::exception_ptr exception)
        {
            mre.set(exception);
        });

    ASSERT_EQ(connection->get_connection_state(), connection_state::connecting);
    state_mre.set();

    try
    {
        mre.get();
        ASSERT_TRUE(false);
    }
    catch (const std::runtime_error & ex)
    {
        ASSERT_STREQ("connecting failed", ex.what());
    }
}

TEST(connection_impl_start, connection_state_is_connected_when_connection_established_succesfully)
{
    auto connection = create_connection();
    auto mre = manual_reset_event<void>();
    connection->start([&mre](std::exception_ptr exception)
        {
            mre.set(exception);
        });
    mre.get();
    ASSERT_EQ(connection->get_connection_state(), connection_state::connected);
}

TEST(connection_impl_start, connection_state_is_disconnected_when_connection_cannot_be_established)
{
    auto http_client = std::shared_ptr<test_http_client>(new test_http_client([](const std::string&, http_request)
        {
            return http_response{ 404, "" };
        }));

    auto connection =
        connection_impl::create(create_uri(), trace_level::none, std::make_shared<memory_log_writer>(),
            [http_client](const signalr_client_config& config) {
                http_client->set_scheduler(config.get_scheduler());
                return http_client;
            }, nullptr);

    auto mre = manual_reset_event<void>();
    connection->start([&mre](std::exception_ptr exception)
        {
            mre.set(exception);
        });

    try
    {
        mre.get();
        ASSERT_TRUE(false);
    }
    catch (...) {}

    ASSERT_EQ(connection->get_connection_state(), connection_state::disconnected);
}

TEST(connection_impl_start, throws_for_invalid_uri)
{
    std::shared_ptr<log_writer> writer(std::make_shared<memory_log_writer>());

    auto websocket_client = create_test_websocket_client();

    auto connection = connection_impl::create(":1\t ä bad_uri&a=b", trace_level::error, writer, create_test_http_client(),
        [websocket_client](const signalr_client_config&) { return websocket_client; });

    auto mre = manual_reset_event<void>();
    connection->start([&mre](std::exception_ptr exception)
        {
            mre.set(exception);
        });

    try
    {
        mre.get();
        ASSERT_TRUE(false);
    }
    catch (const std::exception&)
    {
        // We shouldn't check the exact exception as it would be specific to the http library being used
    }

    ASSERT_EQ(connection->get_connection_state(), connection_state::disconnected);
}

TEST(connection_impl_start, start_sets_id_query_string)
{
    std::shared_ptr<log_writer> writer(std::make_shared<memory_log_writer>());
    std::string query_string;

    auto websocket_client = create_test_websocket_client(
        /* send function */ [](const std::string&, std::function<void(std::exception_ptr)> callback) { callback(std::make_exception_ptr(std::runtime_error("should not be invoked")));  },
        /* connect function */[&query_string](const std::string& url, std::function<void(std::exception_ptr)> callback)
        {
            query_string = url.substr(url.find('?') + 1);
            callback(std::make_exception_ptr(std::runtime_error("connecting failed")));
        });

    auto connection = connection_impl::create(create_uri(""), trace_level::error, writer, create_test_http_client(),
        [websocket_client](const signalr_client_config& config)
        {
            websocket_client->set_config(config);
            return websocket_client;
        });

    auto mre = manual_reset_event<void>();
    connection->start([&mre](std::exception_ptr exception)
        {
            mre.set(exception);
        });

    try
    {
        mre.get();
        ASSERT_TRUE(false);
    }
    catch (const std::runtime_error & ex)
    {
        ASSERT_STREQ("connecting failed", ex.what());
    }

    ASSERT_EQ("id=f7707523-307d-4cba-9abf-3eef701241e8", query_string);
}

TEST(connection_impl_start, start_appends_id_query_string)
{
    std::shared_ptr<log_writer> writer(std::make_shared<memory_log_writer>());
    std::string query_string;

    auto websocket_client = create_test_websocket_client(
        /* send function */ [](const std::string&, std::function<void(std::exception_ptr)> callback) { callback(std::make_exception_ptr(std::runtime_error("should not be invoked")));  },
        /* connect function */[&query_string](const std::string& url, std::function<void(std::exception_ptr)> callback)
        {
            query_string = url.substr(url.find('?') + 1);
            callback(std::make_exception_ptr(std::runtime_error("connecting failed")));
        });

    auto connection = connection_impl::create(create_uri("a=b&c=d"), trace_level::error,
        writer, create_test_http_client(), [websocket_client](const signalr_client_config& config)
        {
            websocket_client->set_config(config);
            return websocket_client;
        });

    auto mre = manual_reset_event<void>();
    connection->start([&mre](std::exception_ptr exception)
        {
            mre.set(exception);
        });

    try
    {
        mre.get();
        ASSERT_TRUE(false);
    }
    catch (const std::runtime_error & ex)
    {
        ASSERT_STREQ("connecting failed", ex.what());
    }

    ASSERT_EQ("a=b&c=d&id=f7707523-307d-4cba-9abf-3eef701241e8", query_string);
}

TEST(connection_impl_start, start_logs_exceptions)
{
    std::shared_ptr<log_writer> writer(std::make_shared<memory_log_writer>());

    auto http_client = std::shared_ptr<test_http_client>(new test_http_client([](const std::string&, http_request)
        {
            return http_response{ 404, "" };
        }));

    auto connection =
        connection_impl::create(create_uri(), trace_level::error, writer,
            [http_client](const signalr_client_config& config) {
                http_client->set_scheduler(config.get_scheduler());
                return http_client;
            }, nullptr);

    auto mre = manual_reset_event<void>();
    connection->start([&mre](std::exception_ptr exception)
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

    ASSERT_TRUE(has_log_entry("[error    ] connection could not be started due to: negotiate failed with status code 404\n", log_entries)) << dump_vector(log_entries);
}

TEST(connection_impl_start, start_propagates_exceptions_from_negotiate)
{
    auto http_client = std::shared_ptr<test_http_client>(new test_http_client([](const std::string&, http_request) -> http_response
        {
            throw custom_exception();
        }));

    auto websocket_client = std::make_shared<test_websocket_client>();

    auto connection =
        connection_impl::create(create_uri(), trace_level::none,
            std::make_shared<memory_log_writer>(), [http_client](const signalr_client_config& config) {
                http_client->set_scheduler(config.get_scheduler());
                return http_client;
            }, [websocket_client](const signalr_client_config&) { return websocket_client; });

    auto mre = manual_reset_event<void>();
    connection->start([&mre](std::exception_ptr exception)
        {
            mre.set(exception);
        });

    try
    {
        mre.get();
        ASSERT_TRUE(false); // exception not thrown
    }
    catch (const custom_exception& e)
    {
        ASSERT_STREQ("custom exception", e.what());
    }
}

TEST(connection_impl_start, start_fails_if_transport_connect_throws)
{
    std::shared_ptr<log_writer> writer(std::make_shared<memory_log_writer>());

    auto websocket_client = create_test_websocket_client(
        /* send function */ [](const std::string&, std::function<void(std::exception_ptr)> callback) { callback(std::make_exception_ptr(std::runtime_error("should not be invoked")));  },
        /* connect function */[](const std::string&, std::function<void(std::exception_ptr)> callback)
        {
            callback(std::make_exception_ptr(std::runtime_error("connecting failed")));
        });

    auto connection = create_connection(websocket_client, writer, trace_level::error);

    auto mre = manual_reset_event<void>();
    connection->start([&mre](std::exception_ptr exception)
        {
            mre.set(exception);
        });

    try
    {
        mre.get();
        ASSERT_TRUE(false); // exception not thrown
    }
    catch (const std::exception & e)
    {
        ASSERT_STREQ("connecting failed", e.what());
    }

    auto log_entries = std::dynamic_pointer_cast<memory_log_writer>(writer)->get_log_entries();
    ASSERT_TRUE(log_entries.size() > 1) << dump_vector(log_entries);

    ASSERT_TRUE(has_log_entry("[error    ] transport could not connect due to: connecting failed\n", log_entries)) << dump_vector(log_entries);
}

TEST(connection_impl_send, send_fails_if_transport_fails_when_receiving_messages)
{
    std::shared_ptr<log_writer> writer(std::make_shared<memory_log_writer>());

    auto websocket_client = create_test_websocket_client(
        /* send function */ [](const std::string&, std::function<void(std::exception_ptr)> callback)
        {
            callback(std::make_exception_ptr(std::runtime_error("send error")));
        });

    auto connection = create_connection(websocket_client, writer, trace_level::error);

    auto mre = manual_reset_event<void>();
    connection->start([&mre](std::exception_ptr exception)
        {
            mre.set(exception);
        });

    mre.get();

    connection->send("message", transfer_format::text, [&mre](std::exception_ptr exception)
        {
            mre.set(exception);
        });

    try
    {
        mre.get();
        ASSERT_TRUE(false); // exception not thrown
    }
    catch (const std::exception & e)
    {
        ASSERT_STREQ("send error", e.what());
    }

    auto log_entries = std::dynamic_pointer_cast<memory_log_writer>(writer)->get_log_entries();

    ASSERT_TRUE(has_log_entry("[error    ] error sending data: send error\n", log_entries)) << dump_vector(log_entries);
}

TEST(connection_impl_start, start_fails_if_negotiate_request_fails)
{
    std::shared_ptr<log_writer> writer(std::make_shared<memory_log_writer>());

    auto http_client = std::shared_ptr<test_http_client>(new test_http_client([](const std::string&, http_request)
        {
            return http_response{ 400, "" };
        }));

    auto websocket_client = std::make_shared<test_websocket_client>();

    auto connection =
        connection_impl::create(create_uri(), trace_level::info, writer,
            [http_client](const signalr_client_config& config) {
                http_client->set_scheduler(config.get_scheduler());
                return http_client;
            }, [websocket_client](const signalr_client_config&) { return websocket_client; });

    auto mre = manual_reset_event<void>();
    connection->start([&mre](std::exception_ptr exception)
        {
            mre.set(exception);
        });

    try
    {
        mre.get();
        ASSERT_TRUE(false); // exception not thrown
    }
    catch (const std::exception & e)
    {
        ASSERT_STREQ("negotiate failed with status code 400", e.what());
    }
}

TEST(connection_impl_start, start_fails_if_negotiate_response_has_error)
{
    std::shared_ptr<log_writer> writer(std::make_shared<memory_log_writer>());

    auto http_client = std::shared_ptr<test_http_client>(new test_http_client([](const std::string& url, http_request)
        {
            auto response_body =
                url.find("/negotiate") != std::string::npos
                ? "{ \"error\": \"bad negotiate\" }"
                : "";

            return http_response{ 200, response_body };
        }));

    auto connect_mre = manual_reset_event<void>();
    auto websocket_client = std::make_shared<test_websocket_client>();
    websocket_client->set_connect_function([&connect_mre](const std::string&, std::function<void(std::exception_ptr)> callback)
        {
            connect_mre.get();
            callback(nullptr);
        });

    auto connection =
        connection_impl::create(create_uri(), trace_level::info, writer,
            [http_client](const signalr_client_config& config) {
                http_client->set_scheduler(config.get_scheduler());
                return http_client;
            }, [websocket_client](const signalr_client_config&) { return websocket_client; });

    auto mre = manual_reset_event<void>();
    connection->start([&mre](std::exception_ptr exception)
        {
            mre.set(exception);
        });

    try
    {
        mre.get();
        ASSERT_TRUE(false); // exception not thrown
    }
    catch (const signalr_exception & e)
    {
        ASSERT_STREQ("bad negotiate", e.what());
    }

    connect_mre.set();
}

TEST(connection_impl_start, start_fails_if_negotiate_response_does_not_have_websockets)
{
    std::shared_ptr<log_writer> writer(std::make_shared<memory_log_writer>());

    auto http_client = std::shared_ptr<test_http_client>(new test_http_client([](const std::string& url, http_request)
        {
            auto response_body =
                url.find("/negotiate") != std::string::npos
                ? "{ \"availableTransports\": [ { \"transport\": \"ServerSentEvents\", \"transferFormats\": [ \"Text\" ] } ] }"
                : "";

            return http_response{ 200, response_body };
        }));

    auto websocket_client = std::make_shared<test_websocket_client>();

    auto connection =
        connection_impl::create(create_uri(), trace_level::info, writer,
            [http_client](const signalr_client_config& config) {
                http_client->set_scheduler(config.get_scheduler());
                return http_client;
            }, [websocket_client](const signalr_client_config&) { return websocket_client; });

    auto mre = manual_reset_event<void>();
    connection->start([&mre](std::exception_ptr exception)
        {
            mre.set(exception);
        });

    try
    {
        mre.get();
        ASSERT_TRUE(false); // exception not thrown
    }
    catch (const signalr_exception & e)
    {
        ASSERT_STREQ("The server does not support WebSockets which is currently the only transport supported by this client.", e.what());
    }
}

TEST(connection_impl_start, start_fails_if_negotiate_response_does_not_have_transports)
{
    std::shared_ptr<log_writer> writer(std::make_shared<memory_log_writer>());

    auto http_client = std::shared_ptr<test_http_client>(new test_http_client([](const std::string& url, http_request)
        {
            auto response_body =
                url.find("/negotiate") != std::string::npos
                ? "{ \"availableTransports\": [ ] }"
                : "";

            return http_response{ 200, response_body };
        }));

    auto websocket_client = std::make_shared<test_websocket_client>();

    auto connection =
        connection_impl::create(create_uri(), trace_level::info, writer,
            [http_client](const signalr_client_config& config) {
                http_client->set_scheduler(config.get_scheduler());
                return http_client;
            }, [websocket_client](const signalr_client_config&) { return websocket_client; });

    auto mre = manual_reset_event<void>();
    connection->start([&mre](std::exception_ptr exception)
        {
            mre.set(exception);
        });

    try
    {
        mre.get();
        ASSERT_TRUE(false); // exception not thrown
    }
    catch (const signalr_exception & e)
    {
        ASSERT_STREQ("The server does not support WebSockets which is currently the only transport supported by this client.", e.what());
    }
}

TEST(connection_impl_start, start_fails_if_negotiate_response_is_invalid)
{
    std::shared_ptr<log_writer> writer(std::make_shared<memory_log_writer>());

    auto http_client = std::shared_ptr<test_http_client>(new test_http_client([](const std::string& url, http_request)
        {
            auto response_body =
                url.find("/negotiate") != std::string::npos
                ? "{ \"availableTransports\": [ "
                : "";

            return http_response{ 200, response_body };
        }));

    auto websocket_client = std::make_shared<test_websocket_client>();

    auto connection =
        connection_impl::create(create_uri(), trace_level::info, writer,
            [http_client](const signalr_client_config& config) {
                http_client->set_scheduler(config.get_scheduler());
                return http_client;
            }, [websocket_client](const signalr_client_config&) { return websocket_client; });

    auto mre = manual_reset_event<void>();
    connection->start([&mre](std::exception_ptr exception)
        {
            mre.set(exception);
        });

    try
    {
        mre.get();
        ASSERT_TRUE(false); // exception not thrown
    }
    catch (const std::runtime_error&) { }
}

TEST(connection_impl_start, negotiate_follows_redirect)
{
    std::shared_ptr<log_writer> writer(std::make_shared<memory_log_writer>());

    auto http_client = std::shared_ptr<test_http_client>(new test_http_client([](const std::string& url, http_request)
        {
            std::string response_body = "";
            if (url.find("/negotiate") != std::string::npos)
            {
                if (url.find("redirected") != std::string::npos)
                {
                    response_body = "{\"connectionId\" : \"f7707523-307d-4cba-9abf-3eef701241e8\", "
                        "\"availableTransports\" : [ { \"transport\": \"WebSockets\", \"transferFormats\": [ \"Text\", \"Binary\" ] } ] }";
                }
                else
                {
                    response_body = "{ \"url\": \"http://redirected\" }";
                }
            }

            return http_response{ 200, response_body };
        }));

    auto websocket_client = std::make_shared<test_websocket_client>();

    std::string connectUrl;
    websocket_client->set_connect_function([&connectUrl](const std::string& url, std::function<void(std::exception_ptr)> callback)
        {
            connectUrl = url;
            callback(nullptr);
        });

    auto connection =
        connection_impl::create(create_uri(), trace_level::info, writer,
            [http_client](const signalr_client_config& config) {
                http_client->set_scheduler(config.get_scheduler());
                return http_client;
            }, [websocket_client](const signalr_client_config& config)
            {
                websocket_client->set_config(config);
                return websocket_client;
            });

    auto mre = manual_reset_event<void>();
    connection->start([&mre](std::exception_ptr exception)
        {
            mre.set(exception);
        });

    mre.get();

    ASSERT_EQ("ws://redirected/?id=f7707523-307d-4cba-9abf-3eef701241e8", connectUrl);
}

TEST(connection_impl_start, negotiate_redirect_uses_accessToken)
{
    std::shared_ptr<log_writer> writer(std::make_shared<memory_log_writer>());
    std::string accessToken;

    auto http_client = std::shared_ptr<test_http_client>(new test_http_client([&accessToken](const std::string& url, http_request request)
        {
            std::string response_body = "";
            if (url.find("/negotiate") != std::string::npos)
            {
                if (url.find("redirected") != std::string::npos)
                {
                    response_body = "{\"connectionId\" : \"f7707523-307d-4cba-9abf-3eef701241e8\", "
                        "\"availableTransports\" : [ { \"transport\": \"WebSockets\", \"transferFormats\": [ \"Text\", \"Binary\" ] } ] }";
                }
                else
                {
                    response_body = "{ \"url\": \"http://redirected\", \"accessToken\": \"secret\" }";
                }
            }

            accessToken = request.headers["Authorization"];
            return http_response{ 200, response_body };
        }));

    auto websocket_client = std::make_shared<test_websocket_client>();

    std::string connectUrl;
    websocket_client->set_connect_function([&connectUrl](const std::string& url, std::function<void(std::exception_ptr)> callback)
        {
            connectUrl = url;
            callback(nullptr);
        });

    auto connection =
        connection_impl::create(create_uri(), trace_level::info, writer,
            [http_client](const signalr_client_config& config) {
                http_client->set_scheduler(config.get_scheduler());
                return http_client;
            }, [websocket_client](const signalr_client_config& config)
            {
                websocket_client->set_config(config);
                return websocket_client;
            });

    auto mre = manual_reset_event<void>();
    connection->start([&mre](std::exception_ptr exception)
        {
            mre.set(exception);
        });

    mre.get();

    ASSERT_EQ("ws://redirected/?id=f7707523-307d-4cba-9abf-3eef701241e8", connectUrl);
    ASSERT_EQ("Bearer secret", accessToken);
}

TEST(connection_impl_start, negotiate_fails_after_too_many_redirects)
{
    std::shared_ptr<log_writer> writer(std::make_shared<memory_log_writer>());

    auto http_client = std::shared_ptr<test_http_client>(new test_http_client([](const std::string& url, http_request)
        {
            std::string response_body = "";
            if (url.find("/negotiate") != std::string::npos)
            {
                // infinite redirect
                response_body = "{ \"url\": \"http://redirected\" }";
            }

            return http_response{ 200, response_body };
        }));

    auto websocket_client = std::make_shared<test_websocket_client>();

    auto connection =
        connection_impl::create(create_uri(), trace_level::info, writer,
            [http_client](const signalr_client_config& config) {
                http_client->set_scheduler(config.get_scheduler());
                return http_client;
            }, [websocket_client](const signalr_client_config&) { return websocket_client; });

    auto mre = manual_reset_event<void>();
    connection->start([&mre](std::exception_ptr exception)
        {
            mre.set(exception);
        });

    try
    {
        mre.get();
        ASSERT_TRUE(false);
    }
    catch (signalr_exception e)
    {
        ASSERT_STREQ("Negotiate redirection limit exceeded.", e.what());
    }
}

TEST(connection_impl_start, negotiate_fails_if_ProtocolVersion_in_response)
{
    std::shared_ptr<log_writer> writer(std::make_shared<memory_log_writer>());

    auto http_client = std::shared_ptr<test_http_client>(new test_http_client([](const std::string& url, http_request)
        {
            std::string response_body = "";
            if (url.find("/negotiate") != std::string::npos)
            {
                response_body = "{\"ProtocolVersion\" : \"\" }";
            }

            return http_response{ 200, response_body };
        }));

    auto websocket_client = std::make_shared<test_websocket_client>();

    auto connection =
        connection_impl::create(create_uri(), trace_level::info, writer,
            [http_client](const signalr_client_config& config) {
                http_client->set_scheduler(config.get_scheduler());
                return http_client;
            }, [websocket_client](const signalr_client_config&) { return websocket_client; });

    auto mre = manual_reset_event<void>();
    connection->start([&mre](std::exception_ptr exception)
        {
            mre.set(exception);
        });

    try
    {
        mre.get();
    }
    catch (signalr_exception e)
    {
        ASSERT_STREQ("Detected a connection attempt to an ASP.NET SignalR Server. This client only supports connecting to an ASP.NET Core SignalR Server. See https://aka.ms/signalr-core-differences for details.", e.what());
    }
}

TEST(connection_impl_start, negotiate_redirect_does_not_overwrite_url)
{
    std::shared_ptr<log_writer> writer(std::make_shared<memory_log_writer>());
    int redirectCount = 0;

    auto http_client = std::shared_ptr<test_http_client>(new test_http_client([&redirectCount](const std::string& url, http_request)
        {
            std::string response_body = "";
            if (url.find("/negotiate") != std::string::npos)
            {
                if (url.find("redirected") != std::string::npos)
                {
                    response_body = "{\"connectionId\" : \"f7707523-307d-4cba-9abf-3eef701241e8\", "
                        "\"availableTransports\" : [ { \"transport\": \"WebSockets\", \"transferFormats\": [ \"Text\", \"Binary\" ] } ] }";
                }
                else
                {
                    response_body = "{ \"url\": \"http://redirected\" }";
                    redirectCount++;
                }
            }

            return http_response{ 200, response_body };
        }));

    auto websocket_client = std::make_shared<test_websocket_client>();

    auto connection =
        connection_impl::create(create_uri(), trace_level::info, writer,
            [http_client](const signalr_client_config& config) {
                http_client->set_scheduler(config.get_scheduler());
                return http_client;
            }, [websocket_client](const signalr_client_config& config)
            {
                websocket_client->set_config(config);
                return websocket_client;
            });

    auto mre = manual_reset_event<void>();
    connection->start([&mre](std::exception_ptr exception)
        {
            mre.set(exception);
        });
    mre.get();
    ASSERT_EQ(1, redirectCount);

    connection->stop([&mre](std::exception_ptr)
        {
            mre.set();
        }, nullptr);
    mre.get();

    connection->start([&mre](std::exception_ptr)
        {
            mre.set();
        });
    mre.get();
    ASSERT_EQ(2, redirectCount);
}

TEST(connection_impl_start, negotiate_redirect_uses_own_query_string)
{
    std::shared_ptr<log_writer> writer(std::make_shared<memory_log_writer>());
    std::string query_string;

    auto websocket_client = create_test_websocket_client(
        /* send function */ [](const std::string&, std::function<void(std::exception_ptr)> callback) { callback(std::make_exception_ptr(std::runtime_error("should not be invoked"))); },
        /* connect function */[&query_string](const std::string& url, std::function<void(std::exception_ptr)> callback)
        {
            query_string = url.substr(url.find('?') + 1);
            callback(std::make_exception_ptr(std::runtime_error("connecting failed")));
        });

    auto http_client = std::shared_ptr<test_http_client>(new test_http_client([](const std::string& url, http_request)
        {
            std::string response_body = "";
            if (url.find("/negotiate") != std::string::npos)
            {
                if (url.find("redirected") != std::string::npos)
                {
                    response_body = "{\"connectionId\" : \"f7707523-307d-4cba-9abf-3eef701241e8\", "
                        "\"availableTransports\" : [ { \"transport\": \"WebSockets\", \"transferFormats\": [ \"Text\", \"Binary\" ] } ] }";
                }
                else
                {
                    response_body = "{ \"url\": \"http://redirected?customQuery=1\" }";
                }
            }

            return http_response{ 200, response_body };
        }));

    auto connection = connection_impl::create(create_uri("a=b&c=d"), trace_level::error, writer,
        [http_client](const signalr_client_config& config) {
            http_client->set_scheduler(config.get_scheduler());
            return http_client;
        }, [websocket_client](const signalr_client_config& config)
        {
            websocket_client->set_config(config);
            return websocket_client;
        });

    auto mre = manual_reset_event<void>();
    connection->start([&mre](std::exception_ptr exception)
        {
            mre.set(exception);
        });

    try
    {
        mre.get();
        ASSERT_TRUE(false);
    }
    catch (const std::runtime_error & ex)
    {
        ASSERT_STREQ("connecting failed", ex.what());
    }

    ASSERT_EQ("customQuery=1&id=f7707523-307d-4cba-9abf-3eef701241e8", query_string);
}

TEST(connection_impl_start, negotiate_with_negotiateVersion_uses_connectionToken)
{
    std::shared_ptr<log_writer> writer(std::make_shared<memory_log_writer>());
    std::string query_string;

    auto websocket_client = create_test_websocket_client(
        /* send function */ [](const std::string&, std::function<void(std::exception_ptr)> callback) { callback(std::make_exception_ptr(std::runtime_error("should not be invoked"))); },
        /* connect function */[&query_string](const std::string& url, std::function<void(std::exception_ptr)> callback)
        {
            query_string = url.substr(url.find('?') + 1);
            callback(std::make_exception_ptr(std::runtime_error("connecting failed")));
        });

    auto http_client = std::shared_ptr<test_http_client>(new test_http_client([](const std::string& url, http_request)
        {
            std::string response_body = "";
            if (url.find("/negotiate") != std::string::npos)
            {
                response_body = "{\"connectionId\" : \"f7707523-307d-4cba-9abf-3eef701241e8\", "
                    "\"connectionToken\":\"1122334455\", \"negotiateVersion\": 1, "
                    "\"availableTransports\" : [ { \"transport\": \"WebSockets\", \"transferFormats\": [ \"Text\", \"Binary\" ] } ] }";
            }

            return http_response{ 200, response_body };
        }));

    auto connection = connection_impl::create(create_uri(), trace_level::error, writer,
        [http_client](const signalr_client_config& config) {
            http_client->set_scheduler(config.get_scheduler());
            return http_client;
        }, [websocket_client](const signalr_client_config& config)
        {
            websocket_client->set_config(config);
            return websocket_client;
        });

    auto mre = manual_reset_event<void>();
    connection->start([&mre](std::exception_ptr exception)
        {
            mre.set(exception);
        });

    try
    {
        mre.get();
        ASSERT_TRUE(false);
    }
    catch (const std::runtime_error & ex)
    {
        ASSERT_STREQ("connecting failed", ex.what());
    }

    ASSERT_EQ("id=1122334455", query_string);
}

TEST(connection_impl_start, correct_connection_id_returned_with_negotiateVersion_one)
{
    std::shared_ptr<log_writer> writer(std::make_shared<memory_log_writer>());

    auto websocket_client = create_test_websocket_client(
        /* send function */ [](const std::string&, std::function<void(std::exception_ptr)> callback) { callback(std::make_exception_ptr(std::runtime_error("should not be invoked"))); },
        /* connect function */[](const std::string&, std::function<void(std::exception_ptr)> callback)
        {
            callback(nullptr);
        });

    auto http_client = std::shared_ptr<test_http_client>(new test_http_client([](const std::string& url, http_request)
        {
            std::string response_body = "";
            if (url.find("/negotiate") != std::string::npos)
            {
                response_body = "{\"connectionId\" : \"f7707523-307d-4cba-9abf-3eef701241e8\", "
                    "\"connectionToken\":\"1122334455\", \"negotiateVersion\": 1, "
                    "\"availableTransports\" : [ { \"transport\": \"WebSockets\", \"transferFormats\": [ \"Text\", \"Binary\" ] } ] }";
            }

            return http_response{ 200, response_body };
        }));

    auto connection = connection_impl::create(create_uri(), trace_level::error, writer,
        [http_client](const signalr_client_config& config) {
            http_client->set_scheduler(config.get_scheduler());
            return http_client;
        }, [websocket_client](const signalr_client_config& config)
        {
            websocket_client->set_config(config);
            return websocket_client;
        });

    auto mre = manual_reset_event<void>();
    connection->start([&mre](std::exception_ptr exception)
        {
            mre.set(exception);
        });

    mre.get();

    ASSERT_EQ("f7707523-307d-4cba-9abf-3eef701241e8", connection->get_connection_id());
}

TEST(connection_impl_start, negotiate_can_be_skipped)
{
    std::shared_ptr<log_writer> writer(std::make_shared<memory_log_writer>());

    bool negotiate_called = false;

    auto http_client = std::shared_ptr<test_http_client>(new test_http_client([&negotiate_called](const std::string& url, http_request request)
        {
            negotiate_called = true;

            auto response_body =
                url.find_first_of("/negotiate") != 0
                ? "{\"connectionId\" : \"f7707523-307d-4cba-9abf-3eef701241e8\", "
                "\"availableTransports\" : [ { \"transport\": \"WebSockets\", \"transferFormats\": [ \"Text\", \"Binary\" ] } ] }"
                : "";

            return http_response{ 200, response_body };
        }));

    auto websocket_client = std::make_shared<test_websocket_client>();

    auto connection =
        connection_impl::create(create_uri(), trace_level::info, writer,
            [http_client](const signalr_client_config& config) {
                http_client->set_scheduler(config.get_scheduler());
                return http_client;
            }, [websocket_client](const signalr_client_config& config)
            {
                websocket_client->set_config(config);
                return websocket_client;
            }, true);

    auto mre = manual_reset_event<void>();
    connection->start([&mre](std::exception_ptr exception)
        {
            mre.set(exception);
        });

    mre.get();

    ASSERT_TRUE(connection->get_connection_state() == connection_state::connected);
    ASSERT_TRUE(connection->get_connection_id().empty());
    ASSERT_FALSE(negotiate_called);
}

TEST(connection_impl_process_response, process_response_logs_messages)
{
    std::shared_ptr<log_writer> writer(std::make_shared<memory_log_writer>());
    auto wait_receive = std::make_shared<cancellation_token>();
    auto websocket_client = create_test_websocket_client();
    auto connection = create_connection(websocket_client, writer, trace_level::debug);

    auto message_mre = manual_reset_event<std::string>();
    connection->set_message_received([&message_mre](std::string&& message)
        {
            message_mre.set(message);
        });

    auto mre = manual_reset_event<void>();
    connection->start([&mre](std::exception_ptr exception)
        {
            mre.set(exception);
        });

    mre.get();

    ASSERT_FALSE(websocket_client->receive_loop_started.wait(1000));
    websocket_client->receive_message("{ }");

    ASSERT_STREQ("{ }", message_mre.get().c_str());

    auto log_entries = std::dynamic_pointer_cast<memory_log_writer>(writer)->get_log_entries();
    ASSERT_FALSE(log_entries.empty());

    ASSERT_TRUE(has_log_entry("[debug    ] processing message: { }\n", log_entries)) << dump_vector(log_entries);
}

TEST(connection_impl_send, message_sent)
{
    std::string actual_message;

    auto websocket_client = create_test_websocket_client(
        /* send function */ [&actual_message](const std::string& message, std::function<void(std::exception_ptr)> callback)
        {
            actual_message = message;
            callback(nullptr);
        });

    auto connection = create_connection(websocket_client);

    const std::string message{ "Test message" };

    auto mre = manual_reset_event<void>();
    connection->start([&mre](std::exception_ptr exception)
        {
            mre.set(exception);
        });

    mre.get();

    connection->send(message, transfer_format::text, [&mre](std::exception_ptr exception)
        {
            mre.set(exception);
        });

    mre.get();

    ASSERT_EQ(message, actual_message);
}

TEST(connection_impl_send, send_throws_if_connection_not_connected)
{
    auto connection =
        connection_impl::create(create_uri(), trace_level::none, std::make_shared<memory_log_writer>());

    auto mre = manual_reset_event<void>();
    connection->send("whatever", transfer_format::text, [&mre](std::exception_ptr exception)
        {
            mre.set(exception);
        });

    try
    {
        mre.get();
        ASSERT_TRUE(false); // exception expected but not thrown
    }
    catch (const signalr_exception & e)
    {
        ASSERT_STREQ("cannot send data when the connection is not in the connected state. current connection state: disconnected", e.what());
    }
}

TEST(connection_impl_send, exceptions_from_send_logged_and_propagated)
{
    std::shared_ptr<log_writer> writer(std::make_shared<memory_log_writer>());
    auto websocket_client = create_test_websocket_client(
        /* send function */ [](const std::string&, std::function<void(std::exception_ptr)> callback)
        {
            callback(std::make_exception_ptr(std::runtime_error("error")));
        });

    auto connection = create_connection(websocket_client, writer, trace_level::error);

    auto mre = manual_reset_event<void>();
    connection->start([&mre](std::exception_ptr exception)
        {
            mre.set(exception);
        });

    mre.get();

    connection->send("Test message", transfer_format::text, [&mre](std::exception_ptr exception)
        {
            mre.set(exception);
        });

    try
    {
        mre.get();

        ASSERT_TRUE(false); // exception expected but not thrown
    }
    catch (const std::runtime_error & e)
    {
        ASSERT_STREQ("error", e.what());
    }

    auto log_entries = std::dynamic_pointer_cast<memory_log_writer>(writer)->get_log_entries();
    ASSERT_FALSE(log_entries.empty());

    ASSERT_TRUE(has_log_entry("[error    ] error sending data: error\n", log_entries)) << dump_vector(log_entries);
}

TEST(connection_impl_set_message_received, callback_invoked_when_message_received)
{
    auto websocket_client = create_test_websocket_client();

    auto connection = create_connection(websocket_client);

    auto message = std::make_shared<std::string>();

    auto message_received_event = std::make_shared<cancellation_token>();
    connection->set_message_received([message, message_received_event](const std::string& m)
        {
            if (m == "Test")
            {
                *message = m;
            }

            if (m == "release")
            {
                message_received_event->cancel();
            }
        });

    auto mre = manual_reset_event<void>();
    connection->start([&mre](std::exception_ptr exception)
        {
            mre.set(exception);
        });

    mre.get();

    websocket_client->receive_message("Test");
    websocket_client->receive_message("release");

    ASSERT_FALSE(message_received_event->wait(5000));

    ASSERT_EQ("Test", *message);
}

TEST(connection_impl_set_message_received, exception_from_callback_caught_and_logged)
{
    auto websocket_client = create_test_websocket_client();

    std::shared_ptr<log_writer> writer(std::make_shared<memory_log_writer>());
    auto connection = create_connection(websocket_client, writer, trace_level::error);

    auto message_received_event = std::make_shared<cancellation_token>();
    connection->set_message_received([message_received_event](const std::string& m)
        {
            if (m == "throw")
            {
                throw std::runtime_error("oops");
            }

            if (m == "release")
            {
                message_received_event->cancel();
            }
        });

    auto mre = manual_reset_event<void>();
    connection->start([&mre](std::exception_ptr exception)
        {
            mre.set(exception);
        });

    mre.get();

    websocket_client->receive_message("throw");
    websocket_client->receive_message("release");

    ASSERT_FALSE(message_received_event->wait(5000));

    auto log_entries = std::dynamic_pointer_cast<memory_log_writer>(writer)->get_log_entries();
    ASSERT_FALSE(log_entries.empty());

    ASSERT_TRUE(has_log_entry("[error    ] message_received callback threw an exception: oops\n", log_entries)) << dump_vector(log_entries);
}

TEST(connection_impl_set_message_received, non_std_exception_from_callback_caught_and_logged)
{
    auto websocket_client = create_test_websocket_client();

    std::shared_ptr<log_writer> writer(std::make_shared<memory_log_writer>());
    auto connection = create_connection(websocket_client, writer, trace_level::error);

    auto message_received_event = std::make_shared<cancellation_token>();
    connection->set_message_received([message_received_event](const std::string& m)
        {
            if (m == "throw")
            {
                throw 42;
            }

            if (m == "release")
            {
                message_received_event->cancel();
            }
        });

    auto mre = manual_reset_event<void>();
    connection->start([&mre](std::exception_ptr exception)
        {
            mre.set(exception);
        });

    mre.get();

    websocket_client->receive_message("throw");
    websocket_client->receive_message("release");

    ASSERT_FALSE(message_received_event->wait(5000));

    auto log_entries = std::dynamic_pointer_cast<memory_log_writer>(writer)->get_log_entries();
    ASSERT_FALSE(log_entries.empty());

    ASSERT_TRUE(has_log_entry("[error    ] message_received callback threw an unknown exception\n", log_entries)) << dump_vector(log_entries);
}

void can_be_set_only_in_disconnected_state(std::function<void(connection_impl*)> callback, const char* expected_exception_message)
{
    auto websocket_client = create_test_websocket_client();
    auto connection = create_connection(websocket_client);

    auto mre = manual_reset_event<void>();
    connection->start([&mre](std::exception_ptr exception)
        {
            mre.set(exception);
        });

    mre.get();

    try
    {
        callback(connection.get());
        ASSERT_TRUE(false); // exception expected but not thrown
    }
    catch (const signalr_exception & e)
    {
        ASSERT_STREQ(expected_exception_message, e.what());
    }
}

TEST(connection_impl_set_configuration, set_message_received_callback_can_be_set_only_in_disconnected_state)
{
    can_be_set_only_in_disconnected_state(
        [](connection_impl* connection) { connection->set_message_received([](const std::string&) {}); },
        "cannot set the callback when the connection is not in the disconnected state. current connection state: connected");
}

TEST(connection_impl_set_configuration, set_disconnected_callback_can_be_set_only_in_disconnected_state)
{
    can_be_set_only_in_disconnected_state(
        [](connection_impl* connection) { connection->set_disconnected([](std::exception_ptr) {}); },
        "cannot set the disconnected callback when the connection is not in the disconnected state. current connection state: connected");
}

TEST(connection_impl_stop, stopping_disconnected_connection_is_no_op)
{
    std::shared_ptr<log_writer> writer{ std::make_shared<memory_log_writer>() };
    auto connection = connection_impl::create(create_uri(), trace_level::verbose, writer);
    auto mre = manual_reset_event<void>();
    connection->stop([&mre](std::exception_ptr exception)
        {
            mre.set(exception);
        }, nullptr);

    mre.get();

    ASSERT_EQ(connection_state::disconnected, connection->get_connection_state());

    auto log_entries = std::dynamic_pointer_cast<memory_log_writer>(writer)->get_log_entries();
    ASSERT_EQ(2U, log_entries.size()) << dump_vector(log_entries);
    ASSERT_TRUE(has_log_entry("[info     ] stopping connection\n", log_entries)) << dump_vector(log_entries);
    ASSERT_TRUE(has_log_entry("[info     ] acquired lock in shutdown()\n", log_entries)) << dump_vector(log_entries);
}

TEST(connection_impl_stop, stopping_disconnecting_connection_returns_canceled_task)
{
    cancellation_token close_event;
    auto writer = std::shared_ptr<log_writer>{ std::make_shared<memory_log_writer>() };

    auto websocket_client = create_test_websocket_client(
        /* send function */ [](const std::string, std::function<void(std::exception_ptr)> callback) { callback(std::make_exception_ptr(std::runtime_error("should not be invoked"))); },
        /* connect function */ [&close_event](const std::string&, std::function<void(std::exception_ptr)> callback) { callback(nullptr); },
        /* close function */ [&close_event](std::function<void(std::exception_ptr)> callback)
        {
            close_event.wait();
            callback(nullptr);
        });

    auto connection = create_connection(websocket_client, writer, trace_level::verbose);

    auto mre = manual_reset_event<void>();
    connection->start([&mre](std::exception_ptr exception)
        {
            mre.set(exception);
        });

    mre.get();

    connection->stop([&mre](std::exception_ptr exception)
        {
            mre.set(exception);
        }, nullptr);

    try
    {
        auto mre_stop = manual_reset_event<void>();
        connection->stop([&mre_stop](std::exception_ptr exception)
            {
                mre_stop.set(exception);
            }, nullptr);

        mre_stop.get();
        ASSERT_FALSE(true); // exception expected but not thrown
    }
    catch (const signalr::canceled_exception&)
    {
    }

    close_event.cancel();
    mre.get();

    ASSERT_EQ(connection_state::disconnected, connection->get_connection_state());

    auto log_entries = std::dynamic_pointer_cast<memory_log_writer>(writer)->get_log_entries();

    ASSERT_TRUE(has_log_entry("[verbose  ] disconnected -> connecting\n", log_entries)) << dump_vector(log_entries);
    ASSERT_TRUE(has_log_entry("[verbose  ] connecting -> connected\n", log_entries)) << dump_vector(log_entries);
    ASSERT_TRUE(has_log_entry("[verbose  ] connected -> disconnecting\n", log_entries)) << dump_vector(log_entries);
    ASSERT_TRUE(has_log_entry("[verbose  ] disconnecting -> disconnected\n", log_entries)) << dump_vector(log_entries);
}

TEST(connection_impl_stop, can_start_and_stop_connection)
{
    auto writer = std::shared_ptr<log_writer>{ std::make_shared<memory_log_writer>() };

    auto websocket_client = create_test_websocket_client();
    auto connection = create_connection(websocket_client, writer, trace_level::verbose);

    auto mre = manual_reset_event<void>();
    connection->start([&mre](std::exception_ptr exception)
        {
            mre.set(exception);
        });

    mre.get();

    connection->stop([&mre](std::exception_ptr exception)
        {
            mre.set(exception);
        }, nullptr);

    mre.get();

    auto log_entries = std::dynamic_pointer_cast<memory_log_writer>(writer)->get_log_entries();

    ASSERT_TRUE(has_log_entry("[verbose  ] disconnected -> connecting\n", log_entries)) << dump_vector(log_entries);
    ASSERT_TRUE(has_log_entry("[verbose  ] connecting -> connected\n", log_entries)) << dump_vector(log_entries);
    ASSERT_TRUE(has_log_entry("[verbose  ] connected -> disconnecting\n", log_entries)) << dump_vector(log_entries);
    ASSERT_TRUE(has_log_entry("[verbose  ] disconnecting -> disconnected\n", log_entries)) << dump_vector(log_entries);
}

TEST(connection_impl_stop, can_start_and_stop_connection_multiple_times)
{
    auto writer = std::shared_ptr<log_writer>{ std::make_shared<memory_log_writer>() };

    {
        auto websocket_client = create_test_websocket_client();
        auto connection = create_connection(websocket_client, writer, trace_level::verbose);

        auto mre = manual_reset_event<void>();
        connection->start([&mre](std::exception_ptr exception)
            {
                mre.set(exception);
            });

        mre.get();

        connection->stop([&mre](std::exception_ptr exception)
            {
                mre.set(exception);
            }, nullptr);

        mre.get();

        connection->start([&mre](std::exception_ptr exception)
            {
                mre.set(exception);
            });

        mre.get();
    }

    auto memory_writer = std::dynamic_pointer_cast<memory_log_writer>(writer);

    // The connection_impl will be destroyed when the last reference to shared_ptr holding is released. This can happen
    // on a different thread in which case the dtor will be invoked on a different thread so we need to wait for this
    // to happen and if it does not the test will fail
    for (int wait_time_ms = 5; wait_time_ms < 1000 && memory_writer->get_log_entries().size() < 16; wait_time_ms <<= 1)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(wait_time_ms));
    }

    auto log_entries = memory_writer->get_log_entries();
    ASSERT_EQ(16U, log_entries.size()) << dump_vector(log_entries);

    auto second_half = std::vector<std::string>(log_entries.begin() + 8, log_entries.end());

    log_entries = std::vector<std::string>(log_entries.begin(), log_entries.begin() + 8);

    ASSERT_TRUE(has_log_entry("[verbose  ] disconnected -> connecting\n", log_entries)) << dump_vector(log_entries);
    ASSERT_TRUE(has_log_entry("[verbose  ] connecting -> connected\n", log_entries)) << dump_vector(log_entries);
    ASSERT_TRUE(has_log_entry("[verbose  ] connected -> disconnecting\n", log_entries)) << dump_vector(log_entries);
    ASSERT_TRUE(has_log_entry("[verbose  ] disconnecting -> disconnected\n", log_entries)) << dump_vector(log_entries);

    ASSERT_TRUE(has_log_entry("[verbose  ] disconnected -> connecting\n", second_half)) << dump_vector(second_half);
    ASSERT_TRUE(has_log_entry("[verbose  ] connecting -> connected\n", second_half)) << dump_vector(second_half);
    ASSERT_TRUE(has_log_entry("[verbose  ] connected -> disconnecting\n", second_half)) << dump_vector(second_half);
    ASSERT_TRUE(has_log_entry("[verbose  ] disconnecting -> disconnected\n", second_half)) << dump_vector(second_half);
}

TEST(connection_impl_stop, dtor_stops_the_connection)
{
    auto writer = std::shared_ptr<log_writer>{ std::make_shared<memory_log_writer>() };

    {
        auto websocket_client = create_test_websocket_client();
        auto connection = create_connection(websocket_client, writer, trace_level::verbose);

        auto mre = manual_reset_event<void>();
        connection->start([&mre](std::exception_ptr exception)
            {
                mre.set(exception);
            });

        mre.get();
    }

    auto memory_writer = std::dynamic_pointer_cast<memory_log_writer>(writer);

    // The connection_impl will be destroyed when the last reference to shared_ptr holding is released. This can happen
    // on a different thread in which case the dtor will be invoked on a different thread so we need to wait for this
    // to happen and if it does not the test will fail
    for (int wait_time_ms = 5; wait_time_ms < 6000 && memory_writer->get_log_entries().size() < 7; wait_time_ms <<= 1)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(wait_time_ms));
    }

    auto log_entries = memory_writer->get_log_entries();

    ASSERT_TRUE(has_log_entry("[verbose  ] disconnected -> connecting\n", log_entries)) << dump_vector(log_entries);
    ASSERT_TRUE(has_log_entry("[verbose  ] connecting -> connected\n", log_entries)) << dump_vector(log_entries);
    ASSERT_TRUE(has_log_entry("[verbose  ] connected -> disconnecting\n", log_entries)) << dump_vector(log_entries);
    ASSERT_TRUE(has_log_entry("[verbose  ] disconnecting -> disconnected\n", log_entries)) << dump_vector(log_entries);
}

TEST(connection_impl_stop, stop_cancels_ongoing_start_request)
{
    auto disconnect_completed_event = std::make_shared<cancellation_token>();

    auto http_client = std::shared_ptr<test_http_client>(new test_http_client([](const std::string& url, http_request)
        {
            auto response_body =
                url.find("/negotiate") != std::string::npos
                ? "{ \"connectionId\" : \"f7707523-307d-4cba-9abf-3eef701241e8\", "
                "\"availableTransports\" : [ { \"transport\": \"WebSockets\", \"transferFormats\": [ \"Text\", \"Binary\" ] } ] }"
                : "";

            return http_response{ 200, response_body };
        }));

    auto wait_for_start_mre = manual_reset_event<void>();

    auto websocket_client = create_test_websocket_client(
        [](const std::string&, std::function<void(std::exception_ptr)> callback) { callback(std::make_exception_ptr(std::exception())); },
        [disconnect_completed_event, &wait_for_start_mre](const std::string&, std::function<void(std::exception_ptr)> callback) {
            wait_for_start_mre.set();
            disconnect_completed_event->wait();
            callback(nullptr);
        });

    auto writer = std::shared_ptr<log_writer>{ std::make_shared<memory_log_writer>() };
    auto connection = connection_impl::create(create_uri(), trace_level::verbose, writer,
        [http_client](const signalr_client_config& config) {
            http_client->set_scheduler(config.get_scheduler());
            return http_client;
        }, [websocket_client](const signalr_client_config& config)
        {
            websocket_client->set_config(config);
            return websocket_client;
        });

    auto mre = manual_reset_event<void>();
    connection->start([&mre](std::exception_ptr exception)
        {
            mre.set(exception);
        });

    wait_for_start_mre.get();

    connection->stop([disconnect_completed_event](std::exception_ptr)
        {
            disconnect_completed_event->cancel();
        }, nullptr);

    try
    {
        mre.get();
        ASSERT_TRUE(false); // exception expected but not thrown
    }
    catch (const canceled_exception&)
    {
    }

    ASSERT_EQ(connection_state::disconnected, connection->get_connection_state());

    auto log_entries = std::dynamic_pointer_cast<memory_log_writer>(writer)->get_log_entries();

    ASSERT_TRUE(has_log_entry("[verbose  ] disconnected -> connecting\n", log_entries)) << dump_vector(log_entries);
    ASSERT_TRUE(has_log_entry("[info     ] [websocket transport] connecting to: ws://stop_cancels_ongoing_start_request/?id=f7707523-307d-4cba-9abf-3eef701241e8\n", log_entries)) << dump_vector(log_entries);
    ASSERT_TRUE(has_log_entry("[info     ] stopping connection\n", log_entries)) << dump_vector(log_entries);
    ASSERT_TRUE(has_log_entry("[info     ] acquired lock in shutdown()\n", log_entries)) << dump_vector(log_entries);
    ASSERT_TRUE(has_log_entry("[info     ] starting the connection has been canceled.\n", log_entries)) << dump_vector(log_entries);
    ASSERT_TRUE(has_log_entry("[verbose  ] connecting -> disconnected\n", log_entries)) << dump_vector(log_entries);
}

TEST(connection_impl_stop, ongoing_start_request_canceled_if_connection_stopped_before_init_message_received)
{
    auto stop_mre = manual_reset_event<void>();
    auto done_mre = manual_reset_event<void>();
    auto http_client = std::shared_ptr<test_http_client>(new test_http_client([&stop_mre, &done_mre](const std::string& url, http_request)
        {
            stop_mre.get();

            auto response_body =
                url.find("/negotiate") != std::string::npos
                ? "{ \"connectionId\" : \"f7707523-307d-4cba-9abf-3eef701241e8\", "
                "\"availableTransports\" : [ { \"transport\": \"WebSockets\", \"transferFormats\": [ \"Text\", \"Binary\" ] } ] }"
                : "";

            done_mre.set();

            return http_response{ 200, response_body };
        }));

    auto websocket_client = create_test_websocket_client();

    auto writer = std::shared_ptr<log_writer>{ std::make_shared<memory_log_writer>() };
    auto connection = connection_impl::create(create_uri(), trace_level::verbose, writer,
        [http_client](const signalr_client_config& config) {
            http_client->set_scheduler(config.get_scheduler());
            return http_client;
        }, [websocket_client](const signalr_client_config& config)
        {
            websocket_client->set_config(config);
            return websocket_client;
        });

    auto mre = manual_reset_event<void>();
    connection->start([&mre](std::exception_ptr exception)
        {
            mre.set(exception);
        });

    connection->stop([&stop_mre](std::exception_ptr exception)
        {
            stop_mre.set(exception);
        }, nullptr);

    try
    {
        mre.get();
        ASSERT_TRUE(false); // exception expected but not thrown
    }
    catch (const std::exception&)
    {
    }

    auto log_entries = std::dynamic_pointer_cast<memory_log_writer>(writer)->get_log_entries();
    ASSERT_EQ(5U, log_entries.size()) << dump_vector(log_entries);
    ASSERT_EQ("[verbose  ] disconnected -> connecting\n", remove_date_from_log_entry(log_entries[0]));
    ASSERT_EQ("[info     ] stopping connection\n", remove_date_from_log_entry(log_entries[1]));
    ASSERT_EQ("[info     ] acquired lock in shutdown()\n", remove_date_from_log_entry(log_entries[2]));
    ASSERT_EQ("[info     ] starting the connection has been canceled.\n", remove_date_from_log_entry(log_entries[3]));
    ASSERT_EQ("[verbose  ] connecting -> disconnected\n", remove_date_from_log_entry(log_entries[4]));

    // avoid AV from accessing stop_mre in callback
    done_mre.get();
}

TEST(connection_impl_stop, stop_invokes_disconnected_callback)
{
    auto websocket_client = create_test_websocket_client();
    auto connection = create_connection(websocket_client);

    auto disconnected_invoked = false;
    connection->set_disconnected([&disconnected_invoked](std::exception_ptr) { disconnected_invoked = true; });

    auto mre = manual_reset_event<void>();
    connection->start([&mre](std::exception_ptr exception)
        {
            mre.set(exception);
        });

    mre.get();

    connection->stop([&mre](std::exception_ptr exception)
        {
            mre.set(exception);
        }, nullptr);

    mre.get();

    ASSERT_TRUE(disconnected_invoked);
}

TEST(connection_impl_stop, std_exception_from_disconnected_callback_caught_and_logged)
{
    auto writer = std::shared_ptr<log_writer>{ std::make_shared<memory_log_writer>() };

    auto websocket_client = create_test_websocket_client();
    auto connection = create_connection(websocket_client, writer, trace_level::error);

    connection->set_disconnected([](std::exception_ptr) { throw std::runtime_error("exception from disconnected"); });

    auto mre = manual_reset_event<void>();
    connection->start([&mre](std::exception_ptr exception)
        {
            mre.set(exception);
        });

    mre.get();

    connection->stop([&mre](std::exception_ptr exception)
        {
            mre.set(exception);
        }, nullptr);

    mre.get();

    auto log_entries = std::dynamic_pointer_cast<memory_log_writer>(writer)->get_log_entries();
    ASSERT_EQ(1U, log_entries.size()) << dump_vector(log_entries);
    ASSERT_TRUE(has_log_entry("[error    ] disconnected callback threw an exception: exception from disconnected\n", log_entries)) << dump_vector(log_entries);
}

TEST(connection_impl_stop, exception_from_disconnected_callback_caught_and_logged)
{
    auto writer = std::shared_ptr<log_writer>{ std::make_shared<memory_log_writer>() };

    auto websocket_client = create_test_websocket_client();
    auto connection = create_connection(websocket_client, writer, trace_level::error);

    connection->set_disconnected([](std::exception_ptr) { throw 42; });

    auto mre = manual_reset_event<void>();
    connection->start([&mre](std::exception_ptr exception)
        {
            mre.set(exception);
        });

    mre.get();

    connection->stop([&mre](std::exception_ptr exception)
        {
            mre.set(exception);
        }, nullptr);

    mre.get();

    auto log_entries = std::dynamic_pointer_cast<memory_log_writer>(writer)->get_log_entries();
    ASSERT_EQ(1U, log_entries.size()) << dump_vector(log_entries);
    ASSERT_TRUE(has_log_entry("[error    ] disconnected callback threw an unknown exception\n", log_entries)) << dump_vector(log_entries);
}

TEST(connection_impl_stop, transport_error_invokes_disconnected_callback)
{
    auto websocket_client = create_test_websocket_client();
    auto connection = create_connection(websocket_client);

    auto disconnect_mre = manual_reset_event<void>();
    connection->set_disconnected([&disconnect_mre](std::exception_ptr ex) { disconnect_mre.set(ex); });

    auto mre = manual_reset_event<void>();
    connection->start([&mre](std::exception_ptr exception)
        {
            mre.set(exception);
        });

    mre.get();

    ASSERT_FALSE(websocket_client->receive_loop_started.wait(1000));
    websocket_client->receive_message(std::make_exception_ptr(custom_exception("error")));

    try
    {
        disconnect_mre.get();
        ASSERT_TRUE(false);
    }
    catch (const custom_exception& ex)
    {
        ASSERT_STREQ("error", ex.what());
    }

    ASSERT_EQ(connection_state::disconnected, connection->get_connection_state());
}

TEST(connection_impl_config, custom_headers_set_in_requests)
{
    auto writer = std::shared_ptr<log_writer>{ std::make_shared<memory_log_writer>() };

    auto http_client = std::shared_ptr<test_http_client>(new test_http_client([](const std::string& url, http_request)
        {
            auto response_body =
                url.find("/negotiate") != std::string::npos
                ? "{\"connectionId\" : \"f7707523-307d-4cba-9abf-3eef701241e8\", "
                "\"availableTransports\" : [ { \"transport\": \"WebSockets\", \"transferFormats\": [ \"Text\", \"Binary\" ] } ] }"
                : "";

            /*auto request = new web_request_stub((unsigned short)200, "OK", response_body);
            request->on_get_response = [](web_request_stub& request)
            {
                auto http_headers = request.m_signalr_client_config.get_http_headers();
                ASSERT_EQ(1U, http_headers.size());
                ASSERT_EQ(_XPLATSTR("42"), http_headers[_XPLATSTR("Answer")]);
            };*/

            return http_response{ 200, response_body };
        }));

    auto websocket_client = create_test_websocket_client();

    auto connection =
        connection_impl::create(create_uri(), trace_level::verbose,
            writer, [http_client](const signalr_client_config& config) {
                http_client->set_scheduler(config.get_scheduler());
                return http_client;
            }, [websocket_client](const signalr_client_config& config)
            {
                websocket_client->set_config(config);
                return websocket_client;
            });

    signalr::signalr_client_config signalr_client_config{};
    auto http_headers = signalr_client_config.get_http_headers();
    http_headers["Answer"] = "42";
    signalr_client_config.set_http_headers(http_headers);
    connection->set_client_config(signalr_client_config);

    auto mre = manual_reset_event<void>();
    connection->start([&mre](std::exception_ptr exception)
        {
            mre.set(exception);
        });

    mre.get();

    connection->stop([&mre](std::exception_ptr exception)
        {
            mre.set(exception);
        }, nullptr);

    mre.get();

    ASSERT_EQ(connection_state::disconnected, connection->get_connection_state());
}

TEST(connection_impl_set_config, config_can_be_set_only_in_disconnected_state)
{
    can_be_set_only_in_disconnected_state(
        [](connection_impl* connection)
        {
            signalr::signalr_client_config signalr_client_config;
            connection->set_client_config(signalr_client_config);
        }, "cannot set client config when the connection is not in the disconnected state. current connection state: connected");
}

TEST(connection_impl_change_state, change_state_logs)
{
    std::shared_ptr<log_writer> writer(std::make_shared<memory_log_writer>());
    auto websocket_client = create_test_websocket_client();
    auto connection = create_connection(websocket_client, writer, trace_level::verbose);

    auto mre = manual_reset_event<void>();
    connection->start([&mre](std::exception_ptr exception)
        {
            mre.set(exception);
        });
    mre.get();

    auto log_entries = std::dynamic_pointer_cast<memory_log_writer>(writer)->get_log_entries();
    ASSERT_FALSE(log_entries.empty());

    ASSERT_TRUE(has_log_entry("[verbose  ] disconnected -> connecting\n", log_entries)) << dump_vector(log_entries);
}

TEST(connection_id, connection_id_is_set_if_start_fails_but_negotiate_request_succeeds)
{
    std::shared_ptr<log_writer> writer(std::make_shared<memory_log_writer>());

    auto websocket_client = create_test_websocket_client(
        /* send function */ [](const std::string, std::function<void(std::exception_ptr)> callback) { callback(std::make_exception_ptr(std::runtime_error("should not be invoked")));  },
        /* connect function */[](const std::string&, std::function<void(std::exception_ptr)> callback)
        {
            callback(std::make_exception_ptr(std::runtime_error("connecting failed")));
        });

    auto connection = create_connection(websocket_client, writer, trace_level::error);

    ASSERT_EQ("", connection->get_connection_id());

    auto mre = manual_reset_event<void>();
    connection->start([&mre](std::exception_ptr exception)
        {
            mre.set(exception);
        });

    try
    {
        mre.get();
        ASSERT_TRUE(false);
    }
    catch (const std::runtime_error & ex)
    {
        ASSERT_STREQ("connecting failed", ex.what());
    }

    ASSERT_EQ("f7707523-307d-4cba-9abf-3eef701241e8", connection->get_connection_id());
}

TEST(connection_id, can_get_connection_id_when_connection_in_connected_state)
{
    auto writer = std::shared_ptr<log_writer>{ std::make_shared<memory_log_writer>() };

    auto websocket_client = create_test_websocket_client();
    auto connection = create_connection(websocket_client, writer, trace_level::verbose);

    std::string connection_id;
    auto mre = manual_reset_event<void>();
    connection->start([&mre, &connection_id, connection](std::exception_ptr exception)
        {
            connection_id = connection->get_connection_id();
            mre.set(exception);
        });

    mre.get();

    connection->stop([&mre](std::exception_ptr exception)
        {
            mre.set(exception);
        }, nullptr);

    mre.get();

    ASSERT_EQ("f7707523-307d-4cba-9abf-3eef701241e8", connection_id);
}

TEST(connection_id, can_get_connection_id_after_connection_has_stopped)
{
    auto writer = std::shared_ptr<log_writer>{ std::make_shared<memory_log_writer>() };

    auto websocket_client = create_test_websocket_client();
    auto connection = create_connection(websocket_client, writer, trace_level::verbose);

    auto mre = manual_reset_event<void>();
    connection->start([&mre](std::exception_ptr exception)
        {
            mre.set(exception);
        });

    mre.get();

    connection->stop([&mre](std::exception_ptr exception)
        {
            mre.set(exception);
        }, nullptr);

    mre.get();

    ASSERT_EQ("f7707523-307d-4cba-9abf-3eef701241e8", connection->get_connection_id());
}

TEST(connection_id, connection_id_reset_when_starting_connection)
{
    auto fail_http_requests = false;

    auto writer = std::shared_ptr<log_writer>{ std::make_shared<memory_log_writer>() };

    auto websocket_client = create_test_websocket_client();

    auto http_client = std::shared_ptr<test_http_client>(new test_http_client([&fail_http_requests](const std::string& url, http_request)
        {
            if (!fail_http_requests) {
                auto response_body =
                    url.find("/negotiate") != std::string::npos
                    ? "{\"connectionId\" : \"f7707523-307d-4cba-9abf-3eef701241e8\", "
                    "\"availableTransports\" : [ { \"transport\": \"WebSockets\", \"transferFormats\": [ \"Text\", \"Binary\" ] } ] }"
                    : "";

                return http_response{ 200, response_body };
            }

            return http_response{ 500, "" };
        }));

    auto connection =
        connection_impl::create(create_uri(), trace_level::none, std::make_shared<memory_log_writer>(),
            [http_client](const signalr_client_config& config) {
                http_client->set_scheduler(config.get_scheduler());
                return http_client;
            }, [websocket_client](const signalr_client_config& config)
            {
                websocket_client->set_config(config);
                return websocket_client;
            });

    auto mre = manual_reset_event<void>();
    connection->start([&mre](std::exception_ptr exception)
        {
            mre.set(exception);
        });

    mre.get();

    connection->stop([&mre](std::exception_ptr exception)
        {
            mre.set(exception);
        }, nullptr);

    mre.get();

    ASSERT_EQ("f7707523-307d-4cba-9abf-3eef701241e8", connection->get_connection_id());

    fail_http_requests = true;

    connection->start([&mre](std::exception_ptr)
        {
            mre.set();
        });

    mre.get();

    ASSERT_EQ("", connection->get_connection_id());
}
