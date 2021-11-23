// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#pragma once

#include <string>
#include <functional>
#include <map>
#include <chrono>
#include "cancellation_token.h"
#include <exception>

namespace signalr
{
    enum class http_method
    {
        GET,
        POST
    };

    class http_request
    {
    public:
        http_request()
            : method(http_method::GET), timeout(std::chrono::seconds(120))
        { }

        http_method method;
        std::map<std::string, std::string> headers;
        std::string content;
        std::chrono::seconds timeout;
    };

    class http_response
    {
    public:
        http_response() {}
        http_response(http_response&& rhs) noexcept : status_code(rhs.status_code), content(std::move(rhs.content)) {}
        http_response(int code, const std::string& content) : status_code(code), content(content) {}
        http_response(const http_response& rhs) : status_code(rhs.status_code), content(rhs.content) {}

        http_response& operator=(const http_response& rhs)
        {
            status_code = rhs.status_code;
            content = rhs.content;

            return *this;
        }

        http_response& operator=(http_response&& rhs) noexcept
        {
            status_code = rhs.status_code;
            content = std::move(rhs.content);

            return *this;
        }

        int status_code = 0;
        std::string content;
    };

    class http_client
    {
    public:
        virtual void send(const std::string& url, http_request& request,
            std::function<void(const http_response&, std::exception_ptr)> callback, cancellation_token token) = 0;

        virtual ~http_client() {}
    };
}
