// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#include "stdafx.h"
#include "json_hub_protocol.h"
#include "test_utils.h"

using namespace signalr;

std::vector<std::pair<std::string, std::shared_ptr<hub_message>>> protocol_test_data
{
    // invocation message without invocation id
    { "{\"arguments\":[1,\"Foo\"],\"target\":\"Target\",\"type\":1}\x1e",
    std::shared_ptr<hub_message>(new invocation_message("", "Target", std::vector<value>{ value(1.f), value("Foo") })) },

    // invocation message with multiple arguments
    { "{\"arguments\":[1,\"Foo\"],\"invocationId\":\"123\",\"target\":\"Target\",\"type\":1}\x1e",
    std::shared_ptr<hub_message>(new invocation_message("123", "Target", std::vector<value>{ value(1.f), value("Foo") })) },

    // invocation message with bool argument
    { "{\"arguments\":[true],\"target\":\"Target\",\"type\":1}\x1e",
    std::shared_ptr<hub_message>(new invocation_message("", "Target", std::vector<value>{ value(true) })) },

    // invocation message with null argument
    { "{\"arguments\":[null],\"target\":\"Target\",\"type\":1}\x1e",
    std::shared_ptr<hub_message>(new invocation_message("", "Target", std::vector<value>{ value(nullptr) })) },

    // invocation message with no arguments
    { "{\"arguments\":[],\"target\":\"Target\",\"type\":1}\x1e",
    std::shared_ptr<hub_message>(new invocation_message("", "Target", std::vector<value>{})) },

    // invocation message with non-ascii string argument
    /*{ "{\"arguments\":[\"\xD7\x9E\xD7\x97\xD7\xA8\xD7\x95\xD7\x96\xD7\xAA\x20\xD7\x9B\xD7\x9C\xD7\xA9\xD7\x94\xD7\x99\"],\"target\":\"Target\",\"type\":1}\x1e",
    std::shared_ptr<hub_message>(new invocation_message("", "Target", std::vector<value>{ value("\xD7\x9E\xD7\x97\xD7\xA8\xD7\x95\xD7\x96\xD7\xAA\x20\xD7\x9B\xD7\x9C\xD7\xA9\xD7\x94\xD7\x99") })) },*/

    // invocation message with object argument
    { "{\"arguments\":[{\"property\":5}],\"target\":\"Target\",\"type\":1}\x1e",
    std::shared_ptr<hub_message>(new invocation_message("", "Target", std::vector<value>{ value(std::map<std::string, value>{ {"property", value(5.f)} }) })) },

    // invocation message with array argument
    { "{\"arguments\":[[1,5]],\"target\":\"Target\",\"type\":1}\x1e",
    std::shared_ptr<hub_message>(new invocation_message("", "Target", std::vector<value>{ value(std::vector<value>{value(1.f), value(5.f)}) })) },

    // ping message
    { "{\"type\":6}\x1e",
    std::shared_ptr<hub_message>(new ping_message()) },

    // completion message with error
    { "{\"error\":\"error\",\"invocationId\":\"1\",\"type\":3}\x1e",
    std::shared_ptr<hub_message>(new completion_message("1", "error", value(), false)) },

    // completion message with result
    { "{\"invocationId\":\"1\",\"result\":42,\"type\":3}\x1e",
    std::shared_ptr<hub_message>(new completion_message("1", "", value(42.f), true)) },

    // completion message with no result or error
    { "{\"invocationId\":\"1\",\"type\":3}\x1e",
    std::shared_ptr<hub_message>(new completion_message("1", "", value(), false)) },

    // completion message with null result
    { "{\"invocationId\":\"1\",\"result\":null,\"type\":3}\x1e",
    std::shared_ptr<hub_message>(new completion_message("1", "", value(), true)) },
};

TEST(json_hub_protocol, write_message)
{
    for (auto& data : protocol_test_data)
    {
        auto output = json_hub_protocol().write_message(data.second.get());
        ASSERT_STREQ(data.first.data(), output.data());
    }
}

TEST(json_hub_protocol, parse_message)
{
    for (auto& data : protocol_test_data)
    {
        auto output = json_hub_protocol().parse_messages(data.first);
        ASSERT_EQ(1, output.size());
        assert_hub_message_equality(data.second.get(), output[0].get());
    }
}

TEST(json_hub_protocol, parsing_field_order_does_not_matter)
{
    invocation_message message = invocation_message("123", "Target", std::vector<value>{value(true)});
    auto output = json_hub_protocol().parse_messages("{\"type\":1,\"invocationId\":\"123\",\"arguments\":[true],\"target\":\"Target\"}\x1e");
    ASSERT_EQ(1, output.size());
    assert_hub_message_equality(&message, output[0].get());

    output = json_hub_protocol().parse_messages("{\"target\":\"Target\",\"invocationId\":\"123\",\"type\":1,\"arguments\":[true]}\x1e");
    ASSERT_EQ(1, output.size());
    assert_hub_message_equality(&message, output[0].get());

    output = json_hub_protocol().parse_messages("{\"target\":\"Target\",\"arguments\":[true],\"type\":1,\"invocationId\":\"123\"}\x1e");
    ASSERT_EQ(1, output.size());
    assert_hub_message_equality(&message, output[0].get());
}

std::vector<std::pair<std::string, std::shared_ptr<hub_message>>> close_messages
{
    // close message with error
    { "{\"error\":\"error\",\"type\":7}\x1e",
    std::shared_ptr<hub_message>(new close_message("error", false)) },

    // close message without error
    { "{\"type\":7}\x1e",
    std::shared_ptr<hub_message>(new close_message("", false)) },

    // close message with error and reconnect
    { "{\"error\":\"error\",\"allowReconnect\":true,\"type\":7}\x1e",
    std::shared_ptr<hub_message>(new close_message("error", true)) },

    // close message with extra property
    { "{\"error\":\"error\",\"extra\":true,\"type\":7}\x1e",
    std::shared_ptr<hub_message>(new close_message("error", false)) },
};

TEST(json_hub_protocol, can_parse_close_message)
{
    for (auto& data : close_messages)
    {
        auto output = json_hub_protocol().parse_messages(data.first);
        ASSERT_EQ(1, output.size());
        assert_hub_message_equality(data.second.get(), output[0].get());
    }
}

TEST(json_hub_protocol, can_serialize_binary)
{
    auto output = json_hub_protocol().write_message(
        std::shared_ptr<hub_message>(new invocation_message("", "Target", std::vector<value>{ value(std::vector<uint8_t>{ 0x67, 0x6F, 0x6F, 0x64, 0x20, 0x64, 0x61, 0x79 }) })).get());

    auto expected = "{\"arguments\":[\"Z29vZCBkYXk=\"],\"target\":\"Target\",\"type\":1}\x1e";
    ASSERT_STREQ(expected, output.data());
}

TEST(json_hub_protocol, can_parse_multiple_messages)
{
    auto output = json_hub_protocol().parse_messages(std::string("{\"arguments\":[],\"target\":\"Target\",\"type\":1}\x1e") +
        "{\"invocationId\":\"1\",\"result\":42,\"type\":3}\x1e");
    ASSERT_EQ(2, output.size());

    invocation_message invocation = invocation_message("", "Target", std::vector<value>{});
    assert_hub_message_equality(&invocation, output[0].get());

    completion_message completion = completion_message("1", "", value(42.f), true);
    assert_hub_message_equality(&completion, output[1].get());
}

TEST(json_hub_protocol, extra_items_ignored_when_parsing)
{
    invocation_message message = invocation_message("", "Target", std::vector<value>{value(true)});
    auto output = json_hub_protocol().parse_messages("{\"type\":1,\"arguments\":[true],\"target\":\"Target\",\"extra\":\"ignored\"}\x1e");
    ASSERT_EQ(1, output.size());
    assert_hub_message_equality(&message, output[0].get());
}

TEST(json_hub_protocol, unknown_message_type_returns_null)
{
    ping_message message = ping_message();
    // adding ping message, just make sure other messages are still being parsed
    auto output = json_hub_protocol().parse_messages("{\"type\":142}\x1e{\"type\":6}\x1e");
    ASSERT_EQ(1, output.size());
    assert_hub_message_equality(&message, output[0].get());
}

std::vector<std::pair<std::string, std::string>> invalid_messages
{
    { "\x1e", "* Line 1, Column 1\n  Syntax error: value, object or array expected.\n* Line 1, Column 1\n  A valid JSON document must be either an array or an object value.\n" },
    { "foo\x1e", "* Line 1, Column 1\n  Syntax error: value, object or array expected.\n* Line 1, Column 2\n  Extra non-whitespace after JSON value.\n" },
    { "[42]\x1e", "Message was not a 'map' type" },
    { "{\x1e", "* Line 1, Column 2\n  Missing '}' or object member name\n" },

    { "{\"arguments\":[],\"target\":\"send\",\"invocationId\":42}\x1e", "Field 'type' not found" },

    // invocation message
    { "{\"type\":1}\x1e", "Field 'target' not found for 'invocation' message" },
    { "{\"type\":1,\"target\":\"send\",\"invocationId\":42}\x1e", "Field 'arguments' not found for 'invocation' message" },
    { "{\"type\":1,\"target\":\"send\",\"arguments\":[],\"invocationId\":42}\x1e", "Expected 'invocationId' to be of type 'string'" },
    { "{\"type\":1,\"target\":\"send\",\"arguments\":42,\"invocationId\":\"42\"}\x1e", "Expected 'arguments' to be of type 'array'" },
    { "{\"type\":1,\"target\":true,\"arguments\":[],\"invocationId\":\"42\"}\x1e", "Expected 'target' to be of type 'string'" },

    // completion message
    { "{\"type\":3}\x1e", "Field 'invocationId' not found for 'completion' message" },
    { "{\"type\":3,\"invocationId\":42}\x1e", "Expected 'invocationId' to be of type 'string'" },
    { "{\"type\":3,\"invocationId\":\"42\",\"error\":[]}\x1e", "Expected 'error' to be of type 'string'" },
    { "{\"type\":3,\"invocationId\":\"42\",\"error\":\"foo\",\"result\":true}\x1e", "The 'error' and 'result' properties are mutually exclusive." },

    // close message
    { "{\"type\":7,\"error\":32}\x1e", "Expected 'error' to be of type 'string'"},
    { "{\"type\":7,\"allowReconnect\":\"q\"}\x1e", "Expected 'allowReconnect' to be of type 'bool'"},
};

TEST(json_hub_protocol, invalid_messages_throw)
{
    for (auto& pair : invalid_messages)
    {
        try
        {
            json_hub_protocol().parse_messages(pair.first);
            ASSERT_TRUE(false);
        }
        catch (const std::exception& exception)
        {
            ASSERT_STREQ(pair.second.data(), exception.what());
        }
    }
}