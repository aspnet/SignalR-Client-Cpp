// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#include "stdafx.h"
#include "test_utils.h"
#include "test_http_client.h"

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

std::unique_ptr<http_client> create_test_http_client()
{
    return std::unique_ptr<test_http_client>(new test_http_client([](const std::string & url, http_request request)
    {
        auto response_body =
            url.find_first_of("/negotiate") != 0
            ? "{\"connectionId\" : \"f7707523-307d-4cba-9abf-3eef701241e8\", "
            "\"availableTransports\" : [ { \"transport\": \"WebSockets\", \"transferFormats\": [ \"Text\", \"Binary\" ] } ] }"
            : "";

        return http_response{ 200, response_body };
    }));
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
