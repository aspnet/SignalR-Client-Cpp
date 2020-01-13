// Copyright (c) .NET Foundation. All rights reserved.
// Licensed under the Apache License, Version 2.0. See License.txt in the project root for license information.

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
        transport_factory(std::shared_ptr<http_client> http_client, std::function<std::shared_ptr<websocket_client>(const signalr_client_config&)> websocket_factory);

        virtual std::shared_ptr<transport> create_transport(transport_type transport_type, const logger& logger,
            const signalr_client_config& signalr_client_config);

        virtual ~transport_factory();
    private:
        std::shared_ptr<http_client> m_http_client;
        std::function<std::shared_ptr<websocket_client>(const signalr_client_config&)> m_websocket_factory;
    };
}
