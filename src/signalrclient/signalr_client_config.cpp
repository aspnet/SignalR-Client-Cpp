// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#include "stdafx.h"
#include "signalrclient/signalr_client_config.h"
#include "signalr_default_scheduler.h"

namespace signalr
{
    signalr_client_config::signalr_client_config()
        : m_scheduler(std::make_shared<signalr_default_scheduler>())
        , m_handshake_timeout(std::chrono::seconds(15))
        , m_server_timeout(std::chrono::seconds(30))
        , m_keepalive_interval(std::chrono::seconds(15))
    {
    }

    const std::map<std::string, std::string>& signalr_client_config::get_http_headers() const noexcept
    {
        return m_http_headers;
    }

    void signalr_client_config::set_http_headers(std::map<std::string, std::string> http_headers) noexcept
    {
        m_http_headers = std::move(http_headers);
    }

    void signalr_client_config::set_scheduler(std::shared_ptr<scheduler> scheduler) noexcept
    {
        if (!scheduler)
        {
            return;
        }

        m_scheduler = std::move(scheduler);
    }

    std::shared_ptr<scheduler> signalr_client_config::get_scheduler() const noexcept
    {
        return m_scheduler;
    }

    void signalr_client_config::set_handshake_timeout(std::chrono::milliseconds timeout)
    {
        if (timeout <= std::chrono::seconds(0))
        {
            throw std::runtime_error("timeout must be greater than 0.");
        }

        m_handshake_timeout = timeout;
    }

    std::chrono::milliseconds signalr_client_config::get_handshake_timeout() const noexcept
    {
        return m_handshake_timeout;
    }

    void signalr_client_config::set_server_timeout(std::chrono::milliseconds timeout)
    {
        if (timeout <= std::chrono::seconds(0))
        {
            throw std::runtime_error("timeout must be greater than 0.");
        }

        m_server_timeout = timeout;
    }

    std::chrono::milliseconds signalr_client_config::get_server_timeout() const noexcept
    {
        return m_server_timeout;
    }

    void signalr_client_config::set_keepalive_interval(std::chrono::milliseconds interval)
    {
        if (interval <= std::chrono::seconds(0))
        {
            throw std::runtime_error("interval must be greater than 0.");
        }

        m_keepalive_interval = interval;
    }

    std::chrono::milliseconds signalr_client_config::get_keepalive_interval() const noexcept
    {
        return m_keepalive_interval;
    }
}
