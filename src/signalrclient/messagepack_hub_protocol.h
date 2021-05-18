// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#pragma once

#ifdef USE_MSGPACK

#include "signalrclient/signalr_value.h"
#include "hub_protocol.h"

namespace signalr
{
    class messagepack_hub_protocol : public hub_protocol
    {
    public:
        std::string write_message(const hub_message*) const;
        std::vector<std::unique_ptr<hub_message>> parse_messages(const std::string&) const;

        const std::string& name() const
        {
            return m_protocol_name;
        }

        int version() const
        {
            return 1;
        }

        signalr::transfer_format transfer_format() const
        {
            return signalr::transfer_format::binary;
        }

        ~messagepack_hub_protocol() {}
    private:
        std::string m_protocol_name = "messagepack";
    };
}

#endif