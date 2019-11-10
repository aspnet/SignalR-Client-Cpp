// Copyright (c) .NET Foundation. All rights reserved.
// Licensed under the Apache License, Version 2.0. See License.txt in the project root for license information.

#pragma once

#include "signalrclient/signalr_value.h"
#include "hub_protocol.h"

namespace signalr
{
    class json_hub_protocol : public hub_protocol
    {
    public:
        std::string write_message(const signalr::value&);
        signalr::value parse_message(const std::string&);
        const std::string& name()
        {
            return m_protocol_name;
        }
    private:
        std::string m_protocol_name = "json";
    };
}