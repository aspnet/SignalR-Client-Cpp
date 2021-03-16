// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#pragma once

#include "signalrclient/signalr_value.h"
#include "hub_protocol.h"
#include <memory>

namespace signalr
{
    namespace handshake
    {
        std::string write_handshake(const std::unique_ptr<hub_protocol>&);
        std::tuple<std::string, signalr::value> parse_handshake(const std::string&);
    }
}