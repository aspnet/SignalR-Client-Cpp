// Copyright (c) .NET Foundation. All rights reserved.
// Licensed under the Apache License, Version 2.0. See License.txt in the project root for license information.

#pragma once

#include "cpprest/ws_client.h"
#include "signalrclient/signalr_client_config.h"
#include "signalrclient/websocket_client.h"

namespace signalr
{
    class default_websocket_client : public websocket_client
    {
    public:
        explicit default_websocket_client(const signalr_client_config& signalr_client_config = {}) noexcept;

        void start(std::string url, transfer_format format, std::function<void(std::exception_ptr)> callback);
        void stop(std::function<void(std::exception_ptr)> callback);
        void send(std::string payload, std::function<void(std::exception_ptr)> callback);
        void receive(std::function<void(std::string, std::exception_ptr)> callback);
    private:
        web::websockets::client::websocket_client m_underlying_client;
    };
}
