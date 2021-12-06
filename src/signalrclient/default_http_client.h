// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#pragma once

#include "signalrclient/signalr_client_config.h"
#include "signalrclient/http_client.h"

namespace signalr
{
    class default_http_client : public http_client
    {
    public:
       explicit default_http_client(const signalr_client_config& config = {}) noexcept;

        void send(const std::string& url, http_request& request,
            std::function<void(const http_response&, std::exception_ptr)> callback, cancellation_token token) override;

    private:
       signalr_client_config m_config;
    };
}
