// Copyright (c) .NET Foundation. All rights reserved.
// Licensed under the Apache License, Version 2.0. See License.txt in the project root for license information.

#pragma once

#include "signalrclient/signalr_value.h"
#include "hub_protocol.h"
#include <memory>

namespace signalr
{
    std::string write_handshake(std::shared_ptr<hub_protocol>);
    std::tuple<std::string, signalr::value> parse_handshake(const std::string&);
}