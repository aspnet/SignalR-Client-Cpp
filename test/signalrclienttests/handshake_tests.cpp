// Copyright (c) .NET Foundation. All rights reserved.
// Licensed under the Apache License, Version 2.0. See License.txt in the project root for license information.

#include "stdafx.h"
#include "handshake_protocol.h"
#include "signalrclient/signalr_exception.h"
#include "json_hub_protocol.h"

using namespace signalr;

TEST(handshake, parses_empty_handshake)
{
    std::string remaining;
    signalr::value handshake;
    std::tie(remaining, handshake) = handshake::parse_handshake("{}\x1e");

    ASSERT_STREQ("", remaining.c_str());
    ASSERT_TRUE(handshake.is_map());
    ASSERT_EQ(0, handshake.as_map().size());
}

TEST(handshake, parses_handshake_with_error)
{
    std::string remaining;
    signalr::value handshake;
    std::tie(remaining, handshake) = handshake::parse_handshake("{\"error\":\"something bad\"}\x1e");

    ASSERT_STREQ("", remaining.c_str());
    ASSERT_TRUE(handshake.is_map());
    ASSERT_STREQ("something bad", handshake.as_map().at("error").as_string().c_str());
}

TEST(handshake, throws_for_incomplete_message)
{
    try
    {
        std::string remaining;
        signalr::value handshake;
        std::tie(remaining, handshake) = handshake::parse_handshake("{}");
        ASSERT_TRUE(false);
    }
    catch (const signalr_exception& ex)
    {
        ASSERT_STREQ("incomplete or invalid message received", ex.what());
    }
}

TEST(handshake, returns_remaining_data)
{
    std::string remaining;
    signalr::value handshake;
    std::tie(remaining, handshake) = handshake::parse_handshake("{}\x1e{\"type\":6}\x1e");

    ASSERT_STREQ("{\"type\":6}\x1e", remaining.c_str());
    ASSERT_TRUE(handshake.is_map());
    ASSERT_EQ(0, handshake.as_map().size());
}

TEST(handshake, extra_fields_are_fine)
{
    std::string remaining;
    signalr::value handshake;
    std::tie(remaining, handshake) = handshake::parse_handshake("{\"extra\":\"blah\"}\x1e");

    ASSERT_STREQ("", remaining.c_str());
    ASSERT_TRUE(handshake.is_map());
    ASSERT_EQ(1, handshake.as_map().size());
}

TEST(handshake, writes_protocol_and_version)
{
    auto protocol = std::shared_ptr<hub_protocol>(new json_hub_protocol());
    auto message = handshake::write_handshake(protocol);

    ASSERT_STREQ("{\"protocol\":\"json\",\"version\":1}\x1e", message.c_str());
}