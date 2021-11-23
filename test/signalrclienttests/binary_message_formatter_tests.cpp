// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#include "stdafx.h"

#ifdef USE_MSGPACK
#include "binary_message_formatter.h"
#include "binary_message_parser.h"

using namespace signalr;

TEST(write_length_prefix, writes_correct_prefix)
{
    // empty message, not a real scenario, but covers an edge case
    std::string payload{};
    binary_message_formatter::write_length_prefix(payload);
    ASSERT_EQ(1, payload.length());
    ASSERT_EQ(0, payload[0]);

    payload = "Hello,\r\nWorld!";
    binary_message_formatter::write_length_prefix(payload);
    ASSERT_EQ(15, payload.length());
    ASSERT_EQ(0x0E, (unsigned char)payload[0]);
    ASSERT_STREQ("Hello,\r\nWorld!", &payload[1]);

    payload = std::string(500, 'c');
    binary_message_formatter::write_length_prefix(payload);
    ASSERT_EQ(502, payload.length());
    ASSERT_EQ(0xF4, (unsigned char)payload[0]);
    ASSERT_EQ(0x03, (unsigned char)payload[1]);

    payload = std::string(16500, 'c');
    binary_message_formatter::write_length_prefix(payload);
    ASSERT_EQ(16503, payload.length());
    ASSERT_EQ(0xF4, (unsigned char)payload[0]);
    ASSERT_EQ(0x80, (unsigned char)payload[1]);
    ASSERT_EQ(0x01, (unsigned char)payload[2]);
}

std::string create_payload(size_t size)
{
    std::string payload;
    payload.reserve(size);

    for (size_t i = 0; i < size; ++i)
    {
        payload.push_back((char)i);
    }

    return payload;
}

void assert_round_trip(std::string original_payload)
{
    size_t length_prefix_length;
    size_t length_of_message;
    std::string payload = original_payload;

    binary_message_formatter::write_length_prefix(payload);
    bool res = binary_message_parser::try_parse_message(reinterpret_cast<const unsigned char*>(payload.data()),
        payload.length(), &length_prefix_length, &length_of_message);

    ASSERT_STREQ(original_payload.data(), payload.data() + length_prefix_length);
}

TEST(formatter_and_parser, round_trips)
{
    std::string payload = create_payload(0);
    assert_round_trip(payload);

    payload = create_payload(1);
    assert_round_trip(payload);

    payload = create_payload(0x7F);
    assert_round_trip(payload);

    payload = create_payload(0x80);
    assert_round_trip(payload);

    payload = create_payload(0x3FFF);
    assert_round_trip(payload);

    payload = create_payload(0x4000);
    assert_round_trip(payload);

    payload = create_payload(0xC0DE);
    assert_round_trip(payload);
}

TEST(write_length_prefix, throws_for_too_large_messages)
{
    try
    {
        std::string str = std::string(INT32_MAX + 1U, ' ');
        binary_message_formatter::write_length_prefix(str);
        ASSERT_TRUE(false);
    }
    catch (const std::exception& ex)
    {
        ASSERT_STREQ("messages over 2GB are not supported.", ex.what());
    }
}

#endif