// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#pragma once

#include "signalrclient/signalr_client_config.h"
#include "negotiation_response.h"
#include "signalrclient/http_client.h"

namespace signalr
{
    namespace negotiate
    {
        void negotiate(std::shared_ptr<http_client> client, const std::string& base_url,
            const signalr_client_config& signalr_client_config,
            std::function<void(negotiation_response&&, std::exception_ptr)> callback, cancellation_token token) noexcept;
    }
}
