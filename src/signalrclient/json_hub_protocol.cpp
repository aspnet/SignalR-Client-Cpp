// Copyright (c) .NET Foundation. All rights reserved.
// Licensed under the Apache License, Version 2.0. See License.txt in the project root for license information.

#include "stdafx.h"
#include "json_hub_protocol.h"

namespace signalr
{
    std::string signalr::json_hub_protocol::write_message(const signalr::value&)
    {
        return std::string();
    }

    signalr::value json_hub_protocol::parse_message(const std::string&)
    {
        return signalr::value();
    }
}