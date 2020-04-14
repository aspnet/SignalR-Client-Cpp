// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#include "stdafx.h"

#ifdef USE_CPPRESTSDK
#include "default_http_client.h"
#include <thread>
#pragma warning (push)
#pragma warning (disable : 5204 4355 4625 4626 4868)
#include <cpprest/http_client.h>
#pragma warning (pop)

namespace signalr
{
    void default_http_client::send(const std::string& url, const http_request& request, std::function<void(const http_response&, std::exception_ptr)> callback)
    {
        web::http::method method;
        if (request.method == http_method::GET)
        {
            method = U("GET");
        }
        else if (request.method == http_method::POST)
        {
            method = U("POST");
        }
        else
        {
            callback(http_response(), std::make_exception_ptr(std::runtime_error("unknown http method")));
            return;
        }

        web::http::http_request http_request;
        http_request.set_method(method);
        http_request.set_body(request.content);
        if (request.headers.size() > 0)
        {
            web::http::http_headers headers;
            for (auto& header : request.headers)
            {
                headers.add(utility::conversions::to_string_t(header.first), utility::conversions::to_string_t(header.second));
            }
            http_request.headers() = headers;
        }

        auto milliseconds = std::chrono::milliseconds(request.timeout);
        pplx::cancellation_token_source cts;
        if (milliseconds.count() != 0)
        {
            pplx::create_task([milliseconds, cts]()
            {
                std::this_thread::sleep_for(milliseconds);
                cts.cancel();
            });
        }

        web::http::client::http_client client(utility::conversions::to_string_t(url));
        client.request(http_request, cts.get_token())
            .then([callback](pplx::task<web::http::http_response> response_task)
        {
            try
            {
                auto http_response = response_task.get();
                auto status_code = http_response.status_code();
                http_response.extract_utf8string()
                    .then([callback, status_code](std::string response_body)
                {
                    signalr::http_response response;
                    response.content = response_body;
                    response.status_code = status_code;
                    callback(std::move(response), nullptr);
                });
            }
            catch (...)
            {
                callback(http_response(), std::current_exception());
            }
        });
    }
}
#endif