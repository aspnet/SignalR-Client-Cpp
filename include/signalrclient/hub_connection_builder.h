// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#pragma once

#include "_exports.h"
#include "hub_connection.h"
#include <memory>
#include "websocket_client.h"
#include "http_client.h"

namespace signalr
{
    class hub_connection_builder
    {
    public:
        SIGNALRCLIENT_API static hub_connection_builder create(const std::string& url);

        SIGNALRCLIENT_API ~hub_connection_builder();

        SIGNALRCLIENT_API hub_connection_builder(const hub_connection_builder&);

        SIGNALRCLIENT_API hub_connection_builder(hub_connection_builder&&) noexcept;

        SIGNALRCLIENT_API hub_connection_builder& operator=(hub_connection_builder&&) noexcept;

        SIGNALRCLIENT_API hub_connection_builder& operator=(const hub_connection_builder&);

        SIGNALRCLIENT_API hub_connection_builder& with_logging(std::shared_ptr<log_writer> logger, trace_level logLevel);

        SIGNALRCLIENT_API hub_connection_builder& with_websocket_factory(std::function<std::shared_ptr<websocket_client>(const signalr_client_config&)> factory);

        SIGNALRCLIENT_API hub_connection_builder& with_http_client(std::shared_ptr<http_client> http_client);

        SIGNALRCLIENT_API hub_connection_builder& skip_negotiation(bool skip = true);

        SIGNALRCLIENT_API hub_connection build();
    private:
        hub_connection_builder(const std::string& url);

        std::string m_url;
        std::shared_ptr<log_writer> m_logger;
        trace_level m_log_level;
        std::function<std::shared_ptr<websocket_client>(const signalr_client_config&)> m_websocket_factory;
        std::shared_ptr<http_client> m_http_client;
        bool m_skip_negotiation;
    };
}
