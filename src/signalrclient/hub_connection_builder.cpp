// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#include "stdafx.h"
#include "signalrclient/hub_connection_builder.h"
#include <stdexcept>
#include "json_hub_protocol.h"
#include "messagepack_hub_protocol.h"

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

    hub_connection_builder& hub_connection_builder::with_http_client_factory(std::function<std::shared_ptr<http_client>(const signalr_client_config&)> http_client_factory)
    {
        m_http_client_factory = http_client_factory;

        return *this;
    }

    hub_connection_builder& hub_connection_builder::skip_negotiation(const bool skip)
    {
        m_skip_negotiation = skip;

        return *this;
    }

#ifdef USE_MSGPACK
    hub_connection_builder& hub_connection_builder::with_messagepack_hub_protocol()
    {
        m_use_messagepack = true;
        return *this;
    }
#endif

    hub_connection hub_connection_builder::build()
    {
#ifndef USE_CPPRESTSDK
        if (m_http_client_factory == nullptr)
        {
            throw std::runtime_error("An http client must be provided using 'with_http_client_factory' on the builder.");
        }

        if (m_websocket_factory == nullptr)
        {
            throw std::runtime_error("A websocket factory must be provided using 'with_websocket_factory' on the builder.");
        }
#endif

        std::unique_ptr<hub_protocol> hub_protocol;

#ifdef USE_MSGPACK
        if (m_use_messagepack)
        {
            hub_protocol = std::unique_ptr<messagepack_hub_protocol>(new messagepack_hub_protocol());
        }
        else
#endif
        {
            hub_protocol = std::unique_ptr<json_hub_protocol>(new json_hub_protocol());
        }

        return hub_connection(m_url, std::move(hub_protocol), m_log_level, m_logger, m_http_client_factory, m_websocket_factory, m_skip_negotiation);
    }
}