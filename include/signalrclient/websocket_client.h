// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#pragma once

#include "transfer_format.h"
#include <functional>

namespace signalr
{
    class websocket_client
    {
    public:
        virtual ~websocket_client() {};

        virtual void start(const std::string& url, std::function<void(std::exception_ptr)> callback) = 0;

        virtual void stop(std::function<void(std::exception_ptr)> callback) = 0;

        virtual void send(const std::string& payload, transfer_format transfer_format, std::function<void(std::exception_ptr)> callback) = 0;

        virtual void receive(std::function<void(const std::string&, std::exception_ptr)> callback) = 0;
    };
}
