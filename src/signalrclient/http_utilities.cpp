// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#include "stdafx.h"
#include "signalrclient/http_client.h"

namespace signalr
{
    http_request::method http_request::get_method() const noexcept
    {
        return m_method;
    }

    const std::map<std::string, std::string>& http_request::get_headers() const noexcept
    {
        return m_headers;
    }

    const std::string& http_request::get_content() const noexcept
    {
        return m_content;
    }

    std::chrono::seconds http_request::get_timeout() const noexcept
    {
        return m_timeout;
    }

    void http_request::set_timeout(std::chrono::seconds timeout) noexcept
    {
        m_timeout = timeout;
    }

    void http_request::set_content(std::string body) noexcept
    {
        m_content = std::move(body);
    }

    void http_request::set_headers(std::map<std::string, std::string> headers) noexcept
    {
        m_headers = std::move(headers);
    }

    void http_request::set_method(http_request::method method) noexcept
    {
        m_method = method;
    }

    int http_response::get_status_code() const noexcept
    {
        return m_status_code;
    }

    const std::string& http_response::get_content() const noexcept
    {
        return m_content;
    }
}