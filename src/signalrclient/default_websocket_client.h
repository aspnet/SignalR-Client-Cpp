// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#pragma once

#ifdef USE_CPPRESTSDK

#pragma warning (push)
#pragma warning (disable : 5204 4355 4625 4626 4868)
#include <cpprest/ws_client.h>
#pragma warning (pop)
#include "signalrclient/signalr_client_config.h"
#include "signalrclient/websocket_client.h"

namespace signalr
{
    class default_websocket_client : public websocket_client
    {
    public:
        explicit default_websocket_client(const signalr_client_config& signalr_client_config = {}) noexcept;

        void start(const std::string& url, std::function<void(std::exception_ptr)> callback);
        void stop(std::function<void(std::exception_ptr)> callback);
        void send(const std::string& payload, transfer_format transfer_format, std::function<void(std::exception_ptr)> callback);
        void receive(std::function<void(const std::string&, std::exception_ptr)> callback);
    private:
        web::websockets::client::websocket_client m_underlying_client;
    };
}

#endif