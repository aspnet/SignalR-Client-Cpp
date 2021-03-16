// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#pragma once

#include "signalrclient/transport_type.h"
#include "signalrclient/transfer_format.h"
#include "logger.h"

namespace signalr
{
    class transport
    {
    public:
        virtual transport_type get_transport_type() const = 0;

        virtual ~transport();

        virtual void start(const std::string& url, std::function<void(std::exception_ptr)> callback) noexcept = 0;
        virtual void stop(std::function<void(std::exception_ptr)> callback) noexcept = 0;
        virtual void on_close(std::function<void(std::exception_ptr)> callback) = 0;

        virtual void send(const std::string& payload, signalr::transfer_format transfer_format, std::function<void(std::exception_ptr)> callback) noexcept = 0;

        virtual void on_receive(std::function<void(std::string&&, std::exception_ptr)> callback) = 0;

    protected:
        transport(const logger& logger);

        logger m_logger;
    };
}
