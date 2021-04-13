// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#include "stdafx.h"
#include "negotiate.h"
#include "test_http_client.h"
#include "test_utils.h"
#include "signalr_default_scheduler.h"

using namespace signalr;

TEST(negotiate, request_created_with_correct_url)
{
    std::string requested_url;
    auto request_factory = std::make_shared<test_http_client>([&requested_url](const std::string& url, http_request request)
    {
        std::string response_body(
            "{ \"connectionId\" : \"f7707523-307d-4cba-9abf-3eef701241e8\", "
            "\"availableTransports\" : [] }");

        requested_url = url;
        return http_response(200, response_body);
    });

    request_factory->set_scheduler(std::make_shared<signalr_default_scheduler>());

    auto mre = manual_reset_event<void>();
    negotiate::negotiate((std::shared_ptr<http_client>)request_factory, "http://fake/signalr", signalr_client_config(),
        [&mre](signalr::negotiation_response&&, std::exception_ptr exception)
        {
            mre.set();
        });

    mre.get();
    ASSERT_EQ("http://fake/signalr/negotiate?negotiateVersion=1", requested_url);
}

TEST(negotiate, negotiation_request_sent_and_response_serialized)
{
    auto request_factory = std::make_shared<test_http_client>([](const std::string&, http_request request)
    {
        std::string response_body(
            "{\"connectionId\" : \"f7707523-307d-4cba-9abf-3eef701241e8\", "
            "\"availableTransports\" : [ { \"transport\": \"WebSockets\", \"transferFormats\": [ \"Text\", \"Binary\" ] },"
            "{ \"transport\": \"ServerSentEvents\", \"transferFormats\": [ \"Text\" ] } ] }");

        return http_response(200, response_body);
    });

    request_factory->set_scheduler(std::make_shared<signalr_default_scheduler>());

    auto mre = manual_reset_event<negotiation_response>();
    negotiate::negotiate((std::shared_ptr<http_client>)request_factory, "http://fake/signalr", signalr_client_config(),
        [&mre](negotiation_response&& response, std::exception_ptr exception)
        {
            mre.set(response);
        });

    auto response = mre.get();

    ASSERT_EQ("f7707523-307d-4cba-9abf-3eef701241e8", response.connectionId);
    ASSERT_EQ("f7707523-307d-4cba-9abf-3eef701241e8", response.connectionToken);
    ASSERT_EQ(2u, response.availableTransports.size());
    ASSERT_EQ(2u, response.availableTransports[0].transfer_formats.size());
    ASSERT_EQ("Text", response.availableTransports[0].transfer_formats[0]);
    ASSERT_EQ("Binary", response.availableTransports[0].transfer_formats[1]);
    ASSERT_EQ(1u, response.availableTransports[1].transfer_formats.size());
    ASSERT_EQ("Text", response.availableTransports[1].transfer_formats[0]);
}

TEST(negotiate, negotiation_response_with_redirect)
{
    auto request_factory = std::make_shared<test_http_client>([](const std::string&, http_request request)
    {
        std::string response_body(
            "{\"url\" : \"http://redirect\", "
            "\"accessToken\" : \"secret\" }");

        return http_response(200, response_body);
    });

    request_factory->set_scheduler(std::make_shared<signalr_default_scheduler>());

    auto mre = manual_reset_event<negotiation_response>();
    negotiate::negotiate((std::shared_ptr<http_client>)request_factory, "http://fake/signalr", signalr_client_config(),
        [&mre](negotiation_response&& response, std::exception_ptr exception)
        {
            mre.set(response);
        });

    auto response = mre.get();

    ASSERT_EQ("http://redirect", response.url);
    ASSERT_EQ("secret", response.accessToken);
}

TEST(negotiate, negotiation_response_with_negotiateVersion)
{
    auto request_factory = std::make_shared<test_http_client>([](const std::string&, http_request request)
        {
            std::string response_body(
                "{\"connectionId\" : \"f7707523-307d-4cba-9abf-3eef701241e8\", "
                "\"negotiateVersion\": 1, "
                "\"connectionToken\": \"42\", "
                "\"availableTransports\" : [ { \"transport\": \"WebSockets\", \"transferFormats\": [ \"Text\", \"Binary\" ] },"
                "{ \"transport\": \"ServerSentEvents\", \"transferFormats\": [ \"Text\" ] } ] }");

            return http_response(200, response_body);
        });

    request_factory->set_scheduler(std::make_shared<signalr_default_scheduler>());

    auto mre = manual_reset_event<negotiation_response>();
    negotiate::negotiate((std::shared_ptr<http_client>)request_factory, "http://fake/signalr", signalr_client_config(),
        [&mre](negotiation_response&& response, std::exception_ptr exception)
        {
            mre.set(response);
        });

    auto response = mre.get();

    ASSERT_EQ("42", response.connectionToken);
    ASSERT_EQ("f7707523-307d-4cba-9abf-3eef701241e8", response.connectionId);
}

TEST(negotiate, negotiation_response_with_future_negotiateVersion)
{
    auto request_factory = std::make_shared<test_http_client>([](const std::string&, http_request request)
        {
            std::string response_body(
                "{\"connectionId\" : \"f7707523-307d-4cba-9abf-3eef701241e8\", "
                "\"negotiateVersion\": 99, "
                "\"connectionToken\": \"42\", "
                "\"availableTransports\" : [ { \"transport\": \"WebSockets\", \"transferFormats\": [ \"Text\", \"Binary\" ] },"
                "{ \"transport\": \"ServerSentEvents\", \"transferFormats\": [ \"Text\" ] } ] }");

            return http_response(200, response_body);
        });

    request_factory->set_scheduler(std::make_shared<signalr_default_scheduler>());

    auto mre = manual_reset_event<negotiation_response>();
    negotiate::negotiate((std::shared_ptr<http_client>)request_factory, "http://fake/signalr", signalr_client_config(),
        [&mre](negotiation_response&& response, std::exception_ptr exception)
        {
            mre.set(response);
        });

    auto response = mre.get();

    ASSERT_EQ("42", response.connectionToken);
    ASSERT_EQ("f7707523-307d-4cba-9abf-3eef701241e8", response.connectionId);
}