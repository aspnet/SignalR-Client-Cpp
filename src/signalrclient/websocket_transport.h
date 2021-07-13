// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#pragma once

#include "transport.h"
#include "logger.h"
#include "signalrclient/websocket_client.h"
#include "connection_impl.h"

namespace signalr
{
    class websocket_transport : public transport, public std::enable_shared_from_this<websocket_transport>
    {
    public:
        static std::shared_ptr<transport> create(const std::function<std::shared_ptr<websocket_client>(const signalr_client_config&)>& websocket_client_factory,
            const signalr_client_config& signalr_client_config, const logger& logger);

        ~websocket_transport();

        websocket_transport(const websocket_transport&) = delete;

        websocket_transport& operator=(const websocket_transport&) = delete;

        transport_type get_transport_type() const noexcept override;

        void start(const std::string& url, std::function<void(std::exception_ptr)> callback) noexcept override;
        void stop(std::function<void(std::exception_ptr)> callback) noexcept override;
        void on_close(std::function<void(std::exception_ptr)> callback) override;

        void send(const std::string& payload, transfer_format transfer_format, std::function<void(std::exception_ptr)> callback) noexcept override;

        void on_receive(std::function<void(std::string&&, std::exception_ptr)>) override;

    private:
        websocket_transport(const std::function<std::shared_ptr<websocket_client>(const signalr_client_config&)>& websocket_client_factory,
            const signalr_client_config& signalr_client_config, const logger& logger);

        std::function<std::shared_ptr<websocket_client>(const signalr_client_config&)> m_websocket_client_factory;
        std::shared_ptr<websocket_client> m_websocket_client;
        std::mutex m_websocket_client_lock;
        std::mutex m_start_stop_lock;
        std::function<void(std::string, std::exception_ptr)> m_process_response_callback;
        std::function<void(std::exception_ptr)> m_close_callback;
        signalr_client_config m_signalr_client_config;

        std::shared_ptr<cancellation_token_source> m_receive_loop_cts;

        void receive_loop(std::shared_ptr<cancellation_token_source> cts);

        std::shared_ptr<websocket_client> safe_get_websocket_client();
    };
}
