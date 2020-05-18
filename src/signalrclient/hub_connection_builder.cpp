// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#include "stdafx.h"
#include "signalrclient/hub_connection_builder.h"
#include <stdexcept>

namespace signalr
{
    hub_connection_builder hub_connection_builder::create(const std::string& url)
    {
        return hub_connection_builder(url);
    }

    hub_connection_builder::hub_connection_builder(const std::string& url)
        : m_url(url), m_logger(nullptr), m_log_level(trace_level::info)
    {
    }

    hub_connection_builder::hub_connection_builder(const hub_connection_builder& rhs)
        : m_url(rhs.m_url), m_logger(rhs.m_logger), m_log_level(rhs.m_log_level)
    {
    }

    hub_connection_builder::hub_connection_builder(hub_connection_builder&& rhs) noexcept
        : m_url(std::move(rhs.m_url)), m_logger(std::move(rhs.m_logger)), m_log_level(rhs.m_log_level)
    {
    }

    hub_connection_builder& hub_connection_builder::operator=(hub_connection_builder&& rhs) noexcept
    {
        m_url = std::move(rhs.m_url);
        m_logger = std::move(rhs.m_logger);
        m_log_level = rhs.m_log_level;

        return *this;
    }

    hub_connection_builder& hub_connection_builder::operator=(const hub_connection_builder& rhs)
    {
        m_url = rhs.m_url;
        m_logger = rhs.m_logger;
        m_log_level = rhs.m_log_level;

        return *this;
    }

    hub_connection_builder::~hub_connection_builder()
    {}

    hub_connection_builder& hub_connection_builder::with_logging(std::shared_ptr<log_writer> logger, trace_level logLevel)
    {
        m_logger = logger;
        m_log_level = logLevel;

        return *this;
    }

    hub_connection_builder& hub_connection_builder::with_websocket_factory(std::function<std::shared_ptr<websocket_client>(const signalr_client_config&)> factory)
    {
        m_websocket_factory = factory;

        return *this;
    }

    hub_connection_builder& hub_connection_builder::with_http_client(std::shared_ptr<http_client> http_client)
    {
        m_http_client = http_client;

        return *this;
    }

    hub_connection_builder& hub_connection_builder::skip_negotiation(const bool skip)
    {
        m_skip_negotiation = skip;

        return *this;
    }

    hub_connection hub_connection_builder::build()
    {
#ifndef USE_CPPRESTSDK
        if (m_http_client == nullptr)
        {
            throw std::runtime_error("An http client must be provided using 'with_http_client' on the builder.");
        }

        if (m_websocket_factory == nullptr)
        {
            throw std::runtime_error("A websocket factory must be provided using 'with_websocket_factory' on the builder.");
        }
#endif

        return hub_connection(m_url, m_log_level, m_logger, m_http_client, m_websocket_factory, m_skip_negotiation);
    }
}