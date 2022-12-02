// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#include "stdafx.h"
#include "signalrclient/http_client.h"

namespace signalr
{
    http_method http_request::get_method() const
    {
        return m_method;
    }

    const std::map<std::string, std::string>& http_request::get_headers() const
    {
        return m_headers;
    }

    const std::string& http_request::get_content() const
    {
        return m_content;
    }

    std::chrono::seconds http_request::get_timeout() const
    {
        return m_timeout;
    }

    void http_request::set_timeout(std::chrono::seconds timeout)
    {
        m_timeout = timeout;
    }

    void http_request::set_content(const std::string& body)
    {
        m_content = body;
    }

    void http_request::set_content(std::string&& body)
    {
        m_content = body;
    }

    void http_request::set_headers(const std::map<std::string, std::string>& headers)
    {
        m_headers = headers;
    }

    void http_request::set_method(http_method method)
    {
        m_method = method;
    }

    int http_response::get_status_code() const
    {
        return m_status_code;
    }

    const std::string& http_response::get_content() const
    {
        return m_content;
    }
}