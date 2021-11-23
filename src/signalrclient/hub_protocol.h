// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#pragma once

#include "signalrclient/signalr_value.h"
#include "signalrclient/transfer_format.h"
#include "message_type.h"
#include <memory>

namespace signalr
{
    struct hub_message
    {
        hub_message(signalr::message_type message_type) : message_type(message_type) {}

        virtual ~hub_message() {}

        signalr::message_type message_type;
    };

    struct hub_invocation_message : hub_message
    {
        hub_invocation_message(const std::string& invocation_id, signalr::message_type message_type)
            : hub_message(message_type), invocation_id(invocation_id)
        { }

        std::string invocation_id;
    };

    struct invocation_message : hub_invocation_message
    {
        invocation_message(const std::string& invocation_id, const std::string& target,
            const std::vector<signalr::value>& args, const std::vector<std::string>& stream_ids = std::vector<std::string>())
            : hub_invocation_message(invocation_id, signalr::message_type::invocation), target(target), arguments(args), stream_ids(stream_ids)
        { }

        invocation_message(std::string&& invocation_id, std::string&& target,
            std::vector<signalr::value>&& args, std::vector<std::string>&& stream_ids = std::vector<std::string>())
            : hub_invocation_message(invocation_id, signalr::message_type::invocation), target(target), arguments(args), stream_ids(stream_ids)
        { }

        std::string target;
        std::vector<signalr::value> arguments;
        std::vector<std::string> stream_ids;
    };

    struct completion_message : hub_invocation_message
    {
        completion_message(const std::string& invocation_id, const std::string& error, const signalr::value& result, bool has_result)
            : hub_invocation_message(invocation_id, signalr::message_type::completion), error(error), result(result), has_result(has_result)
        { }

        completion_message(std::string&& invocation_id, std::string&& error, signalr::value&& result, bool has_result)
            : hub_invocation_message(invocation_id, signalr::message_type::completion), error(error), result(result), has_result(has_result)
        { }

        std::string error;
        signalr::value result;
        bool has_result;
    };

    struct ping_message : hub_message
    {
        ping_message() : hub_message(signalr::message_type::ping) {}
    };

    class hub_protocol
    {
    public:
        virtual std::string write_message(const hub_message*) const = 0;
        virtual std::vector<std::unique_ptr<hub_message>> parse_messages(const std::string&) const = 0;
        virtual const std::string& name() const = 0;
        virtual int version() const = 0;
        virtual signalr::transfer_format transfer_format() const = 0;
        virtual ~hub_protocol() {}
    };
}