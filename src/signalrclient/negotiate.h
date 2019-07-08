// Copyright (c) .NET Foundation. All rights reserved.
// Licensed under the Apache License, Version 2.0. See License.txt in the project root for license information.

#pragma once

#include "signalrclient/signalr_client_config.h"
#include "negotiation_response.h"
#include "signalrclient/http_client.h"

namespace signalr
{
    namespace negotiate
    {
        void negotiate(http_client& client, const std::string& base_url,
            const signalr_client_config& signalr_client_config,
            std::function<void(const negotiation_response&, std::exception_ptr)> callback) noexcept;
    }
}
