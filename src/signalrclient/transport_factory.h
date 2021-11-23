// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#pragma once

#include <memory>
#include "signalrclient/signalr_client_config.h"
#include "signalrclient/transport_type.h"
#include "transport.h"

namespace signalr
{
    class websocket_client;
    class http_client;

    class transport_factory
    {
    public:
        transport_factory(std::function<std::shared_ptr<http_client>(const signalr_client_config&)>,
            std::function<std::shared_ptr<websocket_client>(const signalr_client_config&)> websocket_factory);

        virtual std::shared_ptr<transport> create_transport(transport_type transport_type, const logger& logger,
            const signalr_client_config& signalr_client_config);

        virtual ~transport_factory();
    private:
        std::function<std::shared_ptr<http_client>(const signalr_client_config&)> m_http_client_factory;
        std::function<std::shared_ptr<websocket_client>(const signalr_client_config&)> m_websocket_factory;
    };
}
