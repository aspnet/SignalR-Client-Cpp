// Copyright (c) .NET Foundation. All rights reserved.
// Licensed under the Apache License, Version 2.0. See License.txt in the project root for license information.

#include "stdafx.h"
#include "handshake_protocol.h"
#include "json_helpers.h"
#include "signalrclient/signalr_exception.h"

namespace signalr
{
    namespace handshake
    {
        std::string write_handshake(std::shared_ptr<hub_protocol>& protocol)
        {
            auto map = std::map<std::string, signalr::value>
            {
                { "protocol", signalr::value(protocol->name()) },
                { "version", signalr::value((double)protocol->version()) }
            };
            return utility::conversions::to_utf8string(createJson(signalr::value(std::move(map))).serialize()) + record_separator;
        }

        std::tuple<std::string, signalr::value> parse_handshake(const std::string& response)
        {
            auto pos = response.find(record_separator);
            if (pos == std::string::npos)
            {
                throw signalr_exception("incomplete or invalid message received");
            }
            auto message = response.substr(0, pos);
            auto result = web::json::value::parse(utility::conversions::to_string_t(message));
            auto remaining_data = response.substr(pos + 1);
            return std::forward_as_tuple(remaining_data, createValue(result));
        }
    }
}