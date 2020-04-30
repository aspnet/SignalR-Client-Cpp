// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#include "stdafx.h"
#include "signalrclient/signalr_client_config.h"

namespace signalr
{
#ifdef USE_CPPRESTSDK
    void signalr_client_config::set_proxy(const web::web_proxy &proxy)
    {
        m_http_client_config.set_proxy(proxy);
        m_websocket_client_config.set_proxy(proxy);
    }

    void signalr_client_config::set_credentials(const web::credentials &credentials)
    {
        m_http_client_config.set_credentials(credentials);
        m_websocket_client_config.set_credentials(credentials);
    }

    web::http::client::http_client_config signalr_client_config::get_http_client_config() const
    {
        return m_http_client_config;
    }

    void signalr_client_config::set_http_client_config(const web::http::client::http_client_config& http_client_config)
    {
        m_http_client_config = http_client_config;
    }

    web::websockets::client::websocket_client_config signalr_client_config::get_websocket_client_config() const noexcept
    {
        return m_websocket_client_config;
    }

    void signalr_client_config::set_websocket_client_config(const web::websockets::client::websocket_client_config& websocket_client_config)
    {
        m_websocket_client_config = websocket_client_config;
    }
#endif

    const std::map<std::string, std::string>& signalr_client_config::get_http_headers() const noexcept
    {
        return m_http_headers;
    }

    std::map<std::string, std::string>& signalr_client_config::get_http_headers() noexcept
    {
        return m_http_headers;
    }

    void signalr_client_config::set_http_headers(const std::map<std::string, std::string>& http_headers)
    {
        m_http_headers = http_headers;
    }

    void signalr_client_config::set_scheduler(std::shared_ptr<scheduler> scheduler)
    {
        m_scheduler = scheduler;
    }

    std::shared_ptr<scheduler> signalr_client_config::get_scheduler() const noexcept
    {
        return m_scheduler;
    }
}
