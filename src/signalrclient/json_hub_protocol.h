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
        std::string write_message(const signalr::value&) const;
        std::vector<signalr::value> parse_messages(const std::string&) const;
        const std::string& name() const
        {
            return m_protocol_name;
        }
        int version() const
        {
            return 1;
        }

        ~json_hub_protocol() {}
    private:
        signalr::value parse_message(const std::string&) const;

        std::string m_protocol_name = "json";
    };
}