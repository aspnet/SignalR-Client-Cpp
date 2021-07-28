// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#include "stdafx.h"
#include "test_utils.h"
#include "test_http_client.h"
#include "signalrclient/signalr_client_config.h"

using namespace signalr;

std::string remove_date_from_log_entry(const std::string &log_entry)
{
    // dates are ISO 8601 (e.g. `2014-11-13T06:05:29.452066Z`)
    auto date_end_index = log_entry.find_first_of("Z") + 1;

    // date is followed by a whitespace hence +1
    return log_entry.substr(date_end_index + 1);
}

bool has_log_entry(const std::string& log_entry, const std::vector<std::string>& logs)
{
    for (auto& log : logs)
    {
        if (remove_date_from_log_entry(log) == log_entry)
        {
            return true;
        }
    }
    return false;
}

std::function<std::shared_ptr<signalr::http_client>(const signalr::signalr_client_config&)> create_test_http_client()
{
    return [](const signalr_client_config& config)
    {
        auto client = std::shared_ptr<test_http_client>(new test_http_client([](const std::string& url, http_request request, cancellation_token)
            {
                auto response_body =
                    url.find_first_of("/negotiate") != 0
                    ? "{\"connectionId\" : \"f7707523-307d-4cba-9abf-3eef701241e8\", "
                    "\"availableTransports\" : [ { \"transport\": \"WebSockets\", \"transferFormats\": [ \"Text\", \"Binary\" ] } ] }"
                    : "";

                return http_response{ 200, response_body };
            }));

        client->set_scheduler(config.get_scheduler());
        return client;
    };
}

std::string create_uri()
{
    auto unit_test = ::testing::UnitTest::GetInstance();

    // unit test will be null if this function is not called in a test
    assert(unit_test);

    return std::string("http://")
        .append(unit_test->current_test_info()->name());
}

std::string create_uri(const std::string& query_string)
{
    auto unit_test = ::testing::UnitTest::GetInstance();

    // unit test will be null if this function is not called in a test
    assert(unit_test);

    return std::string("http://")
        .append(unit_test->current_test_info()->name())
        .append("?" + query_string);
}

std::vector<std::string> filter_vector(const std::vector<std::string>& source, const std::string& string)
{
    std::vector<std::string> filtered_entries;
    std::copy_if(source.begin(), source.end(), std::back_inserter(filtered_entries), [&string](const std::string &e)
    {
        return e.find(string) != std::string::npos;
    });
    return filtered_entries;
}

std::string dump_vector(const std::vector<std::string>& source)
{
    std::stringstream ss;
    ss << "Number of entries: " << source.size() << std::endl;
    for (const auto& e : source)
    {
        ss << e;
        if (e.back() != '\n')
        {
            ss << std::endl;
        }
    }

    return ss.str();
}

void assert_signalr_value_equality(const signalr::value& expected, const signalr::value& actual)
{
    ASSERT_EQ(expected.type(), actual.type());
    switch (expected.type())
    {
    case value_type::string:
        ASSERT_STREQ(expected.as_string().data(), actual.as_string().data());
        break;
    case value_type::boolean:
        ASSERT_EQ(expected.as_bool(), actual.as_bool());
        break;
    case value_type::float64:
        ASSERT_DOUBLE_EQ(expected.as_double(), actual.as_double());
        break;
    case value_type::map:
    {
        auto& expected_map = expected.as_map();
        auto& actual_map = actual.as_map();
        ASSERT_EQ(expected_map.size(), actual_map.size());
        for (auto& pair : expected_map)
        {
            const auto& actual_found = actual_map.find(pair.first);
            ASSERT_FALSE(actual_found == actual_map.end());
            assert_signalr_value_equality(pair.second, actual_found->second);
        }
        break;
    }
    case value_type::array:
    {
        auto& expected_array = expected.as_array();
        auto& actual_array = actual.as_array();
        ASSERT_EQ(expected_array.size(), actual_array.size());
        for (auto i = 0; i < expected_array.size(); ++i)
        {
            assert_signalr_value_equality(expected_array[i], actual_array[i]);
        }
        break;
    }
    case value_type::null:
        break;
    default:
        ASSERT_TRUE(false);
        break;
    }
}

void assert_hub_message_equality(signalr::hub_message* expected, signalr::hub_message* actual)
{
    ASSERT_EQ(expected->message_type, actual->message_type);
    switch (expected->message_type)
    {
    case message_type::invocation:
    {
        auto expected_message = reinterpret_cast<invocation_message*>(expected);
        auto actual_message = reinterpret_cast<invocation_message*>(actual);

        ASSERT_STREQ(expected_message->invocation_id.data(), actual_message->invocation_id.data());
        ASSERT_STREQ(expected_message->target.data(), actual_message->target.data());
        assert_signalr_value_equality(expected_message->arguments, actual_message->arguments);
        //stream_ids
        break;
    }
    case message_type::completion:
    {
        auto expected_message = reinterpret_cast<completion_message*>(expected);
        auto actual_message = reinterpret_cast<completion_message*>(actual);

        ASSERT_EQ(expected_message->has_result, actual_message->has_result);
        ASSERT_STREQ(expected_message->invocation_id.data(), actual_message->invocation_id.data());
        ASSERT_STREQ(expected_message->error.data(), actual_message->error.data());
        assert_signalr_value_equality(expected_message->result, actual_message->result);

        break;
    }
    case message_type::ping:
    {
        // No fields on ping messages currently
        break;
    }
    default:
        ASSERT_TRUE(false);
        break;
    }
}