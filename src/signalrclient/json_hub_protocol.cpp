// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#include "stdafx.h"
#include "json_hub_protocol.h"
#include "message_type.h"
#include "json_helpers.h"
#include "signalrclient/signalr_exception.h"

namespace signalr
{
    std::string signalr::json_hub_protocol::write_message(const signalr::value& hub_message) const
    {
        return Json::writeString(getJsonWriter(), createJson(hub_message)) + record_separator;
    }

    std::vector<signalr::value> json_hub_protocol::parse_messages(const std::string& message) const
    {
        std::vector<signalr::value> vec;
        size_t offset = 0;
        auto pos = message.find(record_separator, offset);
        while (pos != std::string::npos)
        {
            auto hub_message = parse_message(message.c_str() + offset, pos - offset);
            vec.push_back(std::move(hub_message));

            offset = pos + 1;
            pos = message.find(record_separator, offset);
        }
        // if offset < message.size()
        // log or close connection because we got an incomplete message
        return vec;
    }

    signalr::value json_hub_protocol::parse_message(const char* begin, size_t length) const
    {
        Json::Value root;
        auto reader = getJsonReader();
        std::string errors;

        if (!reader->parse(begin, begin + length, &root, &errors))
        {
            throw signalr_exception(errors);
        }

        auto value = createValue(root);

        if (!value.is_map())
        {
            throw signalr_exception("Message was not a 'map' type");
        }

        const auto& obj = value.as_map();

        auto found = obj.find("type");
        if (found == obj.end())
        {
            throw signalr_exception("Field 'type' not found");
        }

        switch ((int)found->second.as_double())
        {
        case message_type::invocation:
        {
            found = obj.find("target");
            if (found == obj.end())
            {
                throw signalr_exception("Field 'target' not found for 'invocation' message");
            }
            found = obj.find("arguments");
            if (found == obj.end())
            {
                throw signalr_exception("Field 'arguments' not found for 'invocation' message");
            }

            break;
        }
        // TODO: other message types
        default:
            // Future protocol changes can add message types, old clients can ignore them
            // TODO: null
            break;
        }

        return value;
    }
}