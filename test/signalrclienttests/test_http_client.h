// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#pragma once

#include "signalrclient/http_client.h"
#include "signalrclient/scheduler.h"

using namespace signalr;

class test_http_client : public http_client
{
public:
    test_http_client(std::function<http_response(const std::string& url, http_request request)> create_http_response_fn);
    void send(const std::string& url, const http_request& request, std::function<void(const http_response&, std::exception_ptr)> callback) override;

    void set_scheduler(std::shared_ptr<scheduler> scheduler);
private:
    std::function<http_response(const std::string& url, http_request request)> m_http_response;

    std::shared_ptr<scheduler> m_scheduler;
};
