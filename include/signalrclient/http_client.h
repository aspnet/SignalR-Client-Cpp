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

    class http_request final
    {
    public:
        http_method get_method() const;

        const std::map<std::string, std::string>& get_headers() const;

        const std::string& get_content() const;

        std::chrono::seconds get_timeout() const;
    private:
        friend class negotiate;

        http_method m_method;
        std::map<std::string, std::string> m_headers;
        std::string m_content;
        std::chrono::seconds m_timeout;

        http_request()
            : m_method(http_method::GET), m_timeout(std::chrono::seconds(120))
        { }

        void set_timeout(std::chrono::seconds timeout);
        void set_content(const std::string& body);
        void set_content(std::string&& body);
        void set_headers(const std::map<std::string, std::string>& headers);
        void set_method(http_method method);
    };

    class http_response final
    {
    public:
        http_response() {}
        http_response(http_response&& rhs) noexcept : m_status_code(rhs.m_status_code), m_content(std::move(rhs.m_content)) {}
        http_response(int code, const std::string& content) : m_status_code(code), m_content(content) {}
        http_response(int code, std::string&& content) : m_status_code(code), m_content(std::move(content)) {}
        http_response(const http_response& rhs) : m_status_code(rhs.m_status_code), m_content(rhs.m_content) {}

        http_response& operator=(const http_response& rhs)
        {
            m_status_code = rhs.m_status_code;
            m_content = rhs.m_content;

            return *this;
        }

        http_response& operator=(http_response&& rhs) noexcept
        {
            m_status_code = rhs.m_status_code;
            m_content = std::move(rhs.m_content);

            return *this;
        }

    private:
        friend class negotiate;

        int get_status_code() const;
        const std::string& get_content() const;

        int m_status_code = 0;
        std::string m_content;
    };

    class http_client
    {
    public:
        virtual void send(const std::string& url, http_request& request,
            std::function<void(const http_response&, std::exception_ptr)> callback, cancellation_token token) = 0;

        virtual ~http_client() {}
    };
}
