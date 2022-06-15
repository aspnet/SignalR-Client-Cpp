// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#include "stdafx.h"

#ifdef USE_MSGPACK
#include "messagepack_hub_protocol.h"
#include "test_utils.h"

using namespace signalr;

namespace
{
    std::string string_from_bytes(std::initializer_list<unsigned char> bytes)
    {
        return std::string(reinterpret_cast<const char*>(bytes.begin()), bytes.size());
    }

    std::vector<std::pair<std::string, std::shared_ptr<hub_message>>> protocol_test_data
    {
        // invocation message without invocation id
        { string_from_bytes({0x12, 0x96, 0x01, 0x80, 0xC0, 0xA6, 0x54, 0x61, 0x72, 0x67, 0x65, 0x74, 0x92, 0x01, 0xA3, 0x46, 0x6F, 0x6F, 0x90}),
        std::shared_ptr<hub_message>(new invocation_message("", "Target", std::vector<value>{ value(1.f), value("Foo") })) },

        // invocation message with multiple arguments
        { string_from_bytes({0x15, 0x96, 0x01, 0x80, 0xA3, 0x31, 0x32, 0x33, 0xA6, 0x54, 0x61, 0x72, 0x67, 0x65, 0x74, 0x92, 0x01, 0xA3, 0x46, 0x6F, 0x6F, 0x90}),
        std::shared_ptr<hub_message>(new invocation_message("123", "Target", std::vector<value>{ value(1.f), value("Foo") })) },

        // invocation message with bool argument
        { string_from_bytes({0x0E, 0x96, 0x01, 0x80, 0xC0, 0xA6, 0x54, 0x61, 0x72, 0x67, 0x65, 0x74, 0x91, 0xC3, 0x90}),
        std::shared_ptr<hub_message>(new invocation_message("", "Target", std::vector<value>{ value(true) })) },

        // invocation message with null argument
        { string_from_bytes({0x0E, 0x96, 0x01, 0x80, 0xC0, 0xA6, 0x54, 0x61, 0x72, 0x67, 0x65, 0x74, 0x91, 0xC0, 0x90}),
        std::shared_ptr<hub_message>(new invocation_message("", "Target", std::vector<value>{ value(nullptr) })) },

        // invocation message with no arguments
        { string_from_bytes({0x0D, 0x96, 0x01, 0x80, 0xC0, 0xA6, 0x54, 0x61, 0x72, 0x67, 0x65, 0x74, 0x90, 0x90}),
        std::shared_ptr<hub_message>(new invocation_message("", "Target", std::vector<value>{})) },

        // invocation message with non-ascii string argument
        /*{ "{\"arguments\":[\"\xD7\x9E\xD7\x97\xD7\xA8\xD7\x95\xD7\x96\xD7\xAA\x20\xD7\x9B\xD7\x9C\xD7\xA9\xD7\x94\xD7\x99\"],\"target\":\"Target\",\"type\":1}\x1e",
        std::shared_ptr<hub_message>(new invocation_message("", "Target", std::vector<value>{ value("\xD7\x9E\xD7\x97\xD7\xA8\xD7\x95\xD7\x96\xD7\xAA\x20\xD7\x9B\xD7\x9C\xD7\xA9\xD7\x94\xD7\x99") })) },*/

        // invocation message with object argument
        { string_from_bytes({0x18, 0x96, 0x01, 0x80, 0xC0, 0xA6, 0x54, 0x61, 0x72, 0x67, 0x65, 0x74, 0x91, 0x81, 0xA8, 0x70, 0x72, 0x6F, 0x70, 0x65, 0x72, 0x74, 0x79, 0x05, 0x90}),
        std::shared_ptr<hub_message>(new invocation_message("", "Target", std::vector<value>{ value(std::map<std::string, value>{ {"property", value(5.f)} }) })) },

        // invocation message with array argument
        { string_from_bytes({0x10, 0x96, 0x01, 0x80, 0xC0, 0xA6, 0x54, 0x61, 0x72, 0x67, 0x65, 0x74, 0x91, 0x92, 0x01, 0x05, 0x90}),
        std::shared_ptr<hub_message>(new invocation_message("", "Target", std::vector<value>{ value(std::vector<value>{value(1.f), value(5.f)}) })) },

        // invocation message with binary argument
        { string_from_bytes({0x14, 0x96, 0x01, 0x80, 0xC0, 0xA6, 0x54, 0x61, 0x72, 0x67, 0x65, 0x74, 0x91, 0xC4, 0x05, 0x17, 0x36, 0x45, 0x6D, 0xC8, 0x90}),
        std::shared_ptr<hub_message>(new invocation_message("", "Target", std::vector<value>{ value(std::vector<uint8_t>{23, 54, 69, 109, 200}) })) },

        // ping message
        { string_from_bytes({0x02, 0x91, 0x06}),
        std::shared_ptr<hub_message>(new ping_message()) },

        // completion message with error
        { string_from_bytes({0x0C, 0x95, 0x03, 0x80, 0xA1, 0x31, 0x01, 0xA5, 0x65, 0x72, 0x72, 0x6F, 0x72}),
        std::shared_ptr<hub_message>(new completion_message("1", "error", value(), false)) },

        // completion message with result
        { string_from_bytes({0x07, 0x95, 0x03, 0x80, 0xA1, 0x31, 0x03, 0x2A}),
        std::shared_ptr<hub_message>(new completion_message("1", "", value(42.f), true)) },

        // completion message with no result or error
        { string_from_bytes({0x06, 0x94, 0x03, 0x80, 0xA1, 0x31, 0x02}),
        std::shared_ptr<hub_message>(new completion_message("1", "", value(), false)) },

        // completion message with null result
        { string_from_bytes({0x07, 0x95, 0x03, 0x80, 0xA1, 0x31, 0x03, 0xC0}),
        std::shared_ptr<hub_message>(new completion_message("1", "", value(), true)) },
    };
}

TEST(messagepack_hub_protocol, write_message)
{
    for (auto& data : protocol_test_data)
    {
        auto output = messagepack_hub_protocol().write_message(data.second.get());
        ASSERT_STREQ(data.first.data(), output.data());
    }
}

TEST(messagepack_hub_protocol, parse_message)
{
    for (auto& data : protocol_test_data)
    {
        auto output = messagepack_hub_protocol().parse_messages(data.first);
        ASSERT_EQ(1, output.size());
        assert_hub_message_equality(data.second.get(), output[0].get());
    }
}

TEST(messagepack_hub_protocol, can_parse_multiple_messages)
{
    auto payload = string_from_bytes({ 0x0D, 0x96, 0x01, 0x80, 0xC0, 0xA6, 0x54, 0x61, 0x72, 0x67, 0x65, 0x74, 0x90, 0x90,
        0x07, 0x95, 0x03, 0x80, 0xA1, 0x31, 0x03, 0x2A });
    auto output = messagepack_hub_protocol().parse_messages(payload);
    ASSERT_EQ(2, output.size());

    invocation_message invocation = invocation_message("", "Target", std::vector<value>{});
    assert_hub_message_equality(&invocation, output[0].get());

    completion_message completion = completion_message("1", "", value(42.f), true);
    assert_hub_message_equality(&completion, output[1].get());
}

TEST(messagepack_hub_protocol, extra_items_ignored_when_parsing)
{
    invocation_message message = invocation_message("", "Target", std::vector<value>{value(1.f), value("Foo")});
    auto payload = string_from_bytes({ 0x16, 0x97, 0x01, 0x80, 0xC0, 0xA6, 0x54, 0x61, 0x72, 0x67, 0x65, 0x74, 0x92, 0x01, 0xA3, 0x46, 0x6F, 0x6F, 0x90, 0xA3, 0x46, 0x6F, 0x6F});
    auto output = messagepack_hub_protocol().parse_messages(payload);
    ASSERT_EQ(1, output.size());
    assert_hub_message_equality(&message, output[0].get());
}

TEST(messagepack_hub_protocol, unknown_message_type_returns_null)
{
    ping_message message = ping_message();
    auto payload = string_from_bytes({ 0x04, 0x93, 0x6E, 0x80, 0xC0,
        // adding ping message, just make sure other messages are still being parsed
        0x02, 0x91, 0x06 });
    auto output = messagepack_hub_protocol().parse_messages(payload);
    ASSERT_EQ(1, output.size());
    assert_hub_message_equality(&message, output[0].get());
}

namespace
{
    std::vector<std::pair<std::string, std::string>> invalid_messages
    {
        { string_from_bytes({0x00}), "messagepack object was incomplete" },
        { string_from_bytes({0x01, 0xA0}), "Message was not an 'array' type" },
        { string_from_bytes({0x05}), "partial messages are not supported." },
        { string_from_bytes({0x02, 0x91, 0xA0}), "reading 'type' as int failed" },
        { string_from_bytes({0x0E, 0x96, 0x01, 0x80, 0xC0, 0xA6, 0x54, 0x61, 0x72, 0x67, 0x65, 0x74, 0x92, 0xC0, 0x90}), "messagepack object was incomplete"},

        // invocation message
        { string_from_bytes({0x03, 0x92, 0x01, 0x80}), "invocation message has too few properties" },
        { string_from_bytes({0x0E, 0x96, 0x01, 0x80, 0x04, 0xA6, 0x54, 0x61, 0x72, 0x67, 0x65, 0x74, 0x91, 0xC0, 0x90}), "reading 'invocationId' as string failed"},
        { string_from_bytes({0x0E, 0x96, 0x01, 0x80, 0xC0, 0x96, 0x54, 0x61, 0x72, 0x67, 0x65, 0x74, 0x91, 0xC0, 0x90}), "reading 'target' as string failed"},
        { string_from_bytes({0x0E, 0x96, 0x01, 0x80, 0xC0, 0xA6, 0x54, 0x61, 0x72, 0x67, 0x65, 0x74, 0xA1, 0xC0, 0x90}), "reading 'arguments' as array failed"},

        // completion message
        { string_from_bytes({0x07, 0x95, 0x03, 0x80, 0x91, 0x31, 0x03, 0x2A}), "reading 'invocationId' as string failed"},
        { string_from_bytes({0x07, 0x95, 0x03, 0x80, 0xA1, 0x31, 0xA0, 0x2A}), "reading 'result_kind' as int failed"},
        { string_from_bytes({0x05, 0x93, 0x03, 0x80, 0xA1, 0x31}), "completion message has too few properties"},
        { string_from_bytes({0x06, 0x94, 0x03, 0x80, 0xA1, 0x31, 0x03}), "completion message has too few properties"},
        { string_from_bytes({0x08, 0x95, 0x03, 0x80, 0xA1, 0x31, 0x01, 0x91, 0x03}), "reading 'error' as string failed"},
    };
}

TEST(messagepack_hub_protocol, invalid_messages_throw)
{
    for (auto& pair : invalid_messages)
    {
        try
        {
            messagepack_hub_protocol().parse_messages(pair.first);
            ASSERT_TRUE(false);
        }
        catch (const std::exception& exception)
        {
            ASSERT_STREQ(pair.second.data(), exception.what());
        }
    }
}

#endif