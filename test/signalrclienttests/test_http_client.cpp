// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#include "stdafx.h"
#include "test_http_client.h"

test_http_client::test_http_client(std::function<http_response(const std::string& url, http_request request)> create_http_response_fn)
    : m_http_response(create_http_response_fn)
{
}

void test_http_client::send(const std::string& url, const http_request& request, std::function<void(const http_response&, std::exception_ptr)> callback)
{
    std::thread([this, url, request, callback]()
        {
            http_response response;
            std::exception_ptr exception;
            try
            {
                response = m_http_response(url, request);
            }
            catch (...)
            {
                exception = std::current_exception();
            }

            callback(std::move(response), exception);
        }).detach();
}
