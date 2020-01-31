// Copyright (c) .NET Foundation. All rights reserved.
// Licensed under the Apache License, Version 2.0. See License.txt in the project root for license information.

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