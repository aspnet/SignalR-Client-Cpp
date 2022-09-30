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
    std::string signalr::json_hub_protocol::write_message(const hub_message* hub_message) const
    {
        Json::Value object(Json::ValueType::objectValue);

#pragma warning (push)
#pragma warning (disable: 4061)
        switch (hub_message->message_type)
        {
        case message_type::invocation:
        {
            auto invocation = static_cast<invocation_message const*>(hub_message);
            object["type"] = static_cast<int>(invocation->message_type);
            if (!invocation->invocation_id.empty())
            {
                object["invocationId"] = invocation->invocation_id;
            }
            object["target"] = invocation->target;
            object["arguments"] = createJson(invocation->arguments);
            // TODO: streamIds

            break;
        }
        case message_type::completion:
        {
            auto completion = static_cast<completion_message const*>(hub_message);
            object["type"] = static_cast<int>(completion->message_type);
            object["invocationId"] = completion->invocation_id;
            if (!completion->error.empty())
            {
                object["error"] = completion->error;
            }
            else if (completion->has_result)
            {
                object["result"] = createJson(completion->result);
            }
            break;
        }
        case message_type::ping:
        {
            auto ping = static_cast<ping_message const*>(hub_message);
            object["type"] = static_cast<int>(ping->message_type);
            break;
        }
        // TODO: other message types
        default:
            break;
        }
#pragma warning (pop)

        return Json::writeString(getJsonWriter(), object) + record_separator;
    }

    std::vector<std::unique_ptr<hub_message>> json_hub_protocol::parse_messages(const std::string& message) const
    {
        std::vector<std::unique_ptr<hub_message>> vec;
        size_t offset = 0;
        auto pos = message.find(record_separator, offset);
        while (pos != std::string::npos)
        {
            auto hub_message = parse_message(message.c_str() + offset, pos - offset);
            if (hub_message != nullptr)
            {
                vec.push_back(std::move(hub_message));
            }

            offset = pos + 1;
            pos = message.find(record_separator, offset);
        }
        // if offset < message.size()
        // log or close connection because we got an incomplete message
        return vec;
    }

    std::unique_ptr<hub_message> json_hub_protocol::parse_message(const char* begin, size_t length) const
    {
        Json::Value root;
        auto reader = getJsonReader();
        std::string errors;

        if (!reader->parse(begin, begin + length, &root, &errors))
        {
            throw signalr_exception(errors);
        }

        // TODO: manually go through the json object to avoid short-lived allocations
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

        std::unique_ptr<hub_message> hub_message;

#pragma warning (push)
        // not all cases handled (we have a default so it's fine)
#pragma warning (disable: 4061)
        switch (static_cast<message_type>(static_cast<int>(found->second.as_double())))
        {
        case message_type::invocation:
        {
            found = obj.find("target");
            if (found == obj.end())
            {
                throw signalr_exception("Field 'target' not found for 'invocation' message");
            }
            if (!found->second.is_string())
            {
                throw signalr_exception("Expected 'target' to be of type 'string'");
            }

            found = obj.find("arguments");
            if (found == obj.end())
            {
                throw signalr_exception("Field 'arguments' not found for 'invocation' message");
            }
            if (!found->second.is_array())
            {
                throw signalr_exception("Expected 'arguments' to be of type 'array'");
            }

            std::string invocation_id;
            found = obj.find("invocationId");
            if (found == obj.end())
            {
                invocation_id = "";
            }
            else
            {
                if (!found->second.is_string())
                {
                    throw signalr_exception("Expected 'invocationId' to be of type 'string'");
                }
                invocation_id = found->second.as_string();
            }

            hub_message = std::unique_ptr<signalr::hub_message>(new invocation_message(invocation_id,
                obj.find("target")->second.as_string(), obj.find("arguments")->second.as_array()));

            break;
        }
        case message_type::completion:
        {
            bool has_result = false;
            signalr::value result;
            found = obj.find("result");
            if (found != obj.end())
            {
                has_result = true;
                result = found->second;
            }

            std::string error;
            found = obj.find("error");
            if (found == obj.end())
            {
                error = "";
            }
            else
            {
                if (found->second.is_string())
                {
                    error = found->second.as_string();
                }
                else
                {
                    throw signalr_exception("Expected 'error' to be of type 'string'");
                }
            }

            found = obj.find("invocationId");
            if (found == obj.end())
            {
                throw signalr_exception("Field 'invocationId' not found for 'completion' message");
            }
            else
            {
                if (!found->second.is_string())
                {
                    throw signalr_exception("Expected 'invocationId' to be of type 'string'");
                }
            }

            if (!error.empty() && has_result)
            {
                throw signalr_exception("The 'error' and 'result' properties are mutually exclusive.");
            }

            hub_message = std::unique_ptr<signalr::hub_message>(new completion_message(obj.find("invocationId")->second.as_string(),
                error, result, has_result));

            break;
        }
        case message_type::ping:
        {
            hub_message = std::unique_ptr<signalr::hub_message>(new ping_message());
            break;
        }
        // TODO: other message types
        default:
            // Future protocol changes can add message types, old clients can ignore them
            // TODO: null
            break;
        }
#pragma warning (pop)

        return hub_message;
    }
}