// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#pragma once

#include <vector>
#include <string>

namespace signalr
{
    struct available_transport
    {
        std::string transport;
        std::vector<std::string> transfer_formats;
    };

    struct negotiation_response
    {
        std::string connectionId;
        std::string connectionToken;
        std::vector<available_transport> availableTransports;
        std::string url;
        std::string accessToken;
        std::string error;
    };
}
