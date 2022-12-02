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
    class hub_connection_builder final
    {
    public:
        SIGNALRCLIENT_API hub_connection_builder(std::string url);

        SIGNALRCLIENT_API ~hub_connection_builder();

        SIGNALRCLIENT_API hub_connection_builder(const hub_connection_builder&) = delete;

        SIGNALRCLIENT_API hub_connection_builder(hub_connection_builder&&) noexcept;

        SIGNALRCLIENT_API hub_connection_builder& operator=(hub_connection_builder&&) noexcept;

        SIGNALRCLIENT_API hub_connection_builder& operator=(const hub_connection_builder&) = delete;

        SIGNALRCLIENT_API hub_connection_builder& configure_options(signalr_client_config config) noexcept;

        SIGNALRCLIENT_API hub_connection_builder& with_logging(std::shared_ptr<log_writer> logger, log_level log_level) noexcept;

        SIGNALRCLIENT_API hub_connection_builder& with_websocket_factory(std::function<std::unique_ptr<websocket_client>(const signalr_client_config&)> websocket_factory) noexcept;

        SIGNALRCLIENT_API hub_connection_builder& with_http_client_factory(std::function<std::unique_ptr<http_client>(const signalr_client_config&)> http_client_factory) noexcept;

        SIGNALRCLIENT_API hub_connection_builder& skip_negotiation(bool skip = true) noexcept;

        SIGNALRCLIENT_API hub_connection_builder& with_messagepack_hub_protocol();

        SIGNALRCLIENT_API std::unique_ptr<hub_connection> build();
    private:
        std::string m_url;
        signalr_client_config m_config;
        std::shared_ptr<log_writer> m_logger;
        log_level m_log_level;
        std::function<std::unique_ptr<websocket_client>(const signalr_client_config&)> m_websocket_factory;
        std::function<std::unique_ptr<http_client>(const signalr_client_config&)> m_http_client_factory;
        bool m_skip_negotiation = false;
        bool m_use_messagepack = false;

        bool m_built = false;
    };
}
