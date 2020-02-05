// Copyright (c) .NET Foundation. All rights reserved.
// Licensed under the Apache License, Version 2.0. See License.txt in the project root for license information.

#include "stdafx.h"
#include "transport_factory.h"
#include "default_websocket_client.h"
#include "websocket_transport.h"
#include "signalrclient/websocket_client.h"

namespace signalr
{
    transport_factory::transport_factory(std::shared_ptr<http_client> http_client, std::function<std::shared_ptr<websocket_client>(const signalr_client_config&)> websocket_factory)
        : m_http_client(http_client), m_websocket_factory(websocket_factory)
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
