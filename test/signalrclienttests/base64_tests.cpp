// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#include "stdafx.h"
#include "json_helpers.h"

using namespace signalr;

std::vector<std::pair<std::vector<uint8_t>, std::string>> test_data
{
    { { 49, 48, 51, 57 },
    "MTAzOQ==" },

    { { 83, 101, 99, 114, 101, 116, 84, 117, 110, 110, 101, 108 },
    "U2VjcmV0VHVubmVs" },

    { { 69, 97, 115, 116, 101, 114, 69, 103, 103, 48, 49 },
    "RWFzdGVyRWdnMDE=" },

    { { 255, 201, 193, 55, 90, 199 },
    "/8nBN1rH" },

    { { 251, 201, 193, 255 },
    "+8nB/w==" }
};

TEST(base_encode, encodes_binary_data)
{
    for (auto& data : test_data)
    {
        auto output = base64Encode(data.first);

        ASSERT_STREQ(data.second.data(), output.data());
    }
}