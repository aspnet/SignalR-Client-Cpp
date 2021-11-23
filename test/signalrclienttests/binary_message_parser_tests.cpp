// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#include "stdafx.h"

#ifdef USE_MSGPACK
#include "binary_message_parser.h"
#include <vector>
#include <signalrclient/signalr_exception.h>

using namespace signalr;

TEST(try_parse_message, produces_correct_lengths)
{
    // payload; length of prefix; length of message
    std::vector<std::tuple<std::string, size_t, size_t>> test_cases
    {
        std::tuple<std::string, size_t, size_t>(std::string{ 0x00 }, (size_t)1, (size_t)0),
        std::tuple<std::string, size_t, size_t>(std::string{ 0x03, 0x41, 0x42, 0x43 }, (size_t)1, (size_t)3),
        std::tuple<std::string, size_t, size_t>(std::string{ 0x0B, 0x41, 0x0A, 0x52, 0x0D, 0x43, 0x0D, 0x0A, 0x3B, 0x44, 0x45, 0x46 }, (size_t)1, (size_t)11),
        std::tuple<std::string, size_t, size_t>(std::string{ (char)0x80, 0x01, // Size - 128
                0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
                0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
                0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
                0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,
                0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f,
                0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f,
                0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f,
                0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f}, (size_t)2, (size_t)128),
    };
    for (auto& test_case : test_cases)
    {
        size_t length_prefix_length = 0;
        size_t length_of_message = 0;
        bool res = binary_message_parser::try_parse_message(reinterpret_cast<const unsigned char*>(std::get<0>(test_case).data()),
            std::get<0>(test_case).length(), &length_prefix_length, &length_of_message);

        ASSERT_TRUE(res);
        ASSERT_EQ(std::get<1>(test_case), length_prefix_length);
        ASSERT_EQ(std::get<2>(test_case), length_of_message);
    }
}

TEST(try_parse_message, only_reads_first_message)
{
    std::string payload{/* length: */ 0x01,
        /* body: */ 0x01,
        /* length: */ 0x0E,
        /* body: */ 0x48, 0x65, 0x6C, 0x6C, 0x6F, 0x2C, 0x0D, 0x0A, 0x57, 0x6F, 0x72, 0x6C, 0x64, 0x21 };

    size_t length_prefix_length = 0;
    size_t length_of_message = 0;
    bool res = binary_message_parser::try_parse_message(reinterpret_cast<const unsigned char*>(payload.data()),
        payload.length(), &length_prefix_length, &length_of_message);

    ASSERT_TRUE(res);
    ASSERT_EQ(1, length_prefix_length);
    ASSERT_EQ(1, length_of_message);

    res = binary_message_parser::try_parse_message(reinterpret_cast<const unsigned char*>(payload.data()) + length_prefix_length + length_of_message,
        payload.length() - length_of_message - length_prefix_length, &length_prefix_length, &length_of_message);

    ASSERT_TRUE(res);
    ASSERT_EQ(1, length_prefix_length);
    ASSERT_EQ(14, length_of_message);
}

TEST(try_parse_message, throws_for_large_messages)
{
    std::vector<std::string> payloads
    {
        { (char)0xFF, (char)0xFF, (char)0xFF, (char)0xFF, (char)0xFF },
        { (char)0x80, (char)0x80, (char)0x80, (char)0x80, 0x08 }, // 2GB + 1
        { (char)0xFF, (char)0xFF, (char)0xFF, (char)0xFF, (char)0xFF, (char)0xFF }

    };

    for (auto& payload : payloads)
    {
        size_t length_prefix_length = 0;
        size_t length_of_message = 0;

        try
        {
            bool res = binary_message_parser::try_parse_message(reinterpret_cast<const unsigned char*>(payload.data()),
                payload.length(), &length_prefix_length, &length_of_message);
            ASSERT_TRUE(false);
        }
        catch (const signalr_exception& exception)
        {
            ASSERT_STREQ("messages over 2GB are not supported.", exception.what());
        }
    }
}

TEST(try_parse_message, throws_for_partial_payloads)
{
    std::vector<std::string> payloads
    {
        { 0x04, (char)0xAB, (char)0xCD, (char)0xEF },
        { (char)0xFF, (char)0xFF, (char)0xFF, (char)0xFF, 0x07 }, // 2GB
        { (char)0x80 } // size is cut
    };

    for (auto& payload : payloads)
    {
        size_t length_prefix_length = 0;
        size_t length_of_message = 0;
        try
        {
            bool res = binary_message_parser::try_parse_message(reinterpret_cast<const unsigned char*>(payload.data()),
                payload.length(), &length_prefix_length, &length_of_message);
            ASSERT_TRUE(false);
        }
        catch (const signalr_exception& exception)
        {
            ASSERT_STREQ("partial messages are not supported.", exception.what());
        }
    }
}

#endif