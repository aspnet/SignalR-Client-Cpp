// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#pragma once

#include "signalrclient/signalr_value.h"

namespace signalr
{
    class hub_protocol
    {
    public:
        virtual std::string write_message(const signalr::value&) const = 0;
        virtual std::vector<signalr::value> parse_messages(const std::string&) const = 0;
        virtual const std::string& name() const = 0;
        virtual int version() const = 0;
        virtual ~hub_protocol() {}
    };
}