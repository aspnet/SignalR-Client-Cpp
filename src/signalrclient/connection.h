// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#pragma once

#include <memory>
#include <functional>
#include "signalrclient/connection_state.h"
#include "signalrclient/trace_level.h"
#include "signalrclient/log_writer.h"
#include "signalrclient/signalr_client_config.h"
#include "signalrclient/transfer_format.h"
#include "signalrclient/http_client.h"
#include "signalrclient/websocket_client.h"

namespace signalr
{
    class connection_impl;

    class connection
    {
    public:
        typedef std::function<void(std::string&&)> message_received_handler;

        explicit connection(const std::string& url, trace_level trace_level = trace_level::info, std::shared_ptr<log_writer> log_writer = nullptr,
            std::function<std::shared_ptr<http_client>(const signalr_client_config&)> http_client_factory = nullptr,
            std::function<std::shared_ptr<websocket_client>(const signalr_client_config&)> websocket_factory = nullptr, bool skip_negotiation = false);

        ~connection();

        connection(const connection&) = delete;

        connection& operator=(const connection&) = delete;

        void start(std::function<void(std::exception_ptr)> callback) noexcept;

        void send(const std::string& data, transfer_format transfer_format, std::function<void(std::exception_ptr)> callback) noexcept;

        void set_message_received(const message_received_handler& message_received_callback);
        void set_disconnected(const std::function<void(std::exception_ptr)>& disconnected_callback);

        void set_client_config(const signalr_client_config& config);

        void stop(std::function<void(std::exception_ptr)> callback, std::exception_ptr exception) noexcept;

        connection_state get_connection_state() const noexcept;
        std::string get_connection_id() const;

    private:
        // The recommended smart pointer to use when doing pImpl is the `std::unique_ptr`. However
        // we are capturing the m_pImpl instance in the lambdas used by tasks which can outlive
        // the connection instance. Using `std::shared_ptr` guarantees that we won't be using
        // a deleted object if the task is run after the `connection` instance goes away.
        std::shared_ptr<connection_impl> m_pImpl;
    };
}
