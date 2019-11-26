// Copyright (c) .NET Foundation. All rights reserved.
// Licensed under the Apache License, Version 2.0. See License.txt in the project root for license information.

#pragma once

#include "signalrclient/http_client.h"

namespace signalr
{
    class default_http_client : public http_client
    {
    public:
        void send(const std::string& url, const http_request& request, std::function<void(const http_response&, std::exception_ptr)> callback) override;
    };
}
