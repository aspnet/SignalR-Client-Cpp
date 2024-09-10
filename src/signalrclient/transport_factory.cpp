// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#include "stdafx.h"
#include "transport_factory.h"
#include "websocket_transport.h"
#include "signalrclient/websocket_client.h"
#include <stdexcept>

namespace signalr
{
    transport_factory::transport_factory(std::function<std::shared_ptr<http_client>(const signalr_client_config&)> http_client_factory,
        std::function<std::shared_ptr<websocket_client>(const signalr_client_config&)> websocket_factory)
        : m_http_client_factory(http_client_factory), m_websocket_factory(websocket_factory)
    {}

    std::shared_ptr<transport> transport_factory::create_transport(transport_type transport_type, const logger& logger,
        const signalr_client_config& signalr_client_config)
    {
        if (transport_type == signalr::transport_type::websockets)
        {
            return websocket_transport::create(m_websocket_factory, signalr_client_config,
                logger);
        }

        throw std::runtime_error("not implemented");
    }

    transport_factory::~transport_factory()
    { }
}
