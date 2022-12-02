// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#pragma once

#include "_exports.h"
#include <memory>
#include <functional>
#include "connection_state.h"
#include "log_level.h"
#include "log_writer.h"
#include "signalr_client_config.h"
#include "signalr_value.h"

namespace signalr
{
    class http_client;
    class websocket_client;
    class hub_connection_impl;
    class hub_connection_builder;
    class hub_protocol;

    class hub_connection
    {
    public:
        SIGNALRCLIENT_API ~hub_connection();

        hub_connection(const hub_connection&) = delete;

        hub_connection& operator=(const hub_connection&) = delete;

        SIGNALRCLIENT_API hub_connection(hub_connection&&) noexcept;

        SIGNALRCLIENT_API hub_connection& operator=(hub_connection&&) noexcept;

        SIGNALRCLIENT_API void start(std::function<void(std::exception_ptr)> callback) noexcept;
        SIGNALRCLIENT_API void stop(std::function<void(std::exception_ptr)> callback) noexcept;

        SIGNALRCLIENT_API connection_state get_connection_state() const;
        SIGNALRCLIENT_API const std::string& get_connection_id() const;

        SIGNALRCLIENT_API void on_disconnected(std::function<void (std::exception_ptr)> disconnected_callback);

        //SIGNALRCLIENT_API void __cdecl set_client_config(const signalr_client_config& config);

        SIGNALRCLIENT_API void on(const std::string& event_name, std::function<void (const std::vector<signalr::value>&)> handler);

        SIGNALRCLIENT_API void invoke(const std::string& method_name, const std::vector<signalr::value>& arguments = std::vector<signalr::value>(), std::function<void(const signalr::value&, std::exception_ptr)> callback = [](const signalr::value&, std::exception_ptr) {}) noexcept;

        SIGNALRCLIENT_API void send(const std::string& method_name, const std::vector<signalr::value>& arguments = std::vector<signalr::value>(), std::function<void(std::exception_ptr)> callback = [](std::exception_ptr) {}) noexcept;

    private:
        friend class hub_connection_builder;

        explicit hub_connection(const std::string& url, std::unique_ptr<hub_protocol>&& hub_protocol, signalr_client_config config,
            log_level log_level = log_level::info, std::shared_ptr<log_writer> log_writer = nullptr,
            std::function<std::unique_ptr<http_client>(const signalr_client_config&)> http_client_factory = nullptr,
            std::function<std::unique_ptr<websocket_client>(const signalr_client_config&)> websocket_factory = nullptr,
            bool skip_negotiation = false);

        std::shared_ptr<hub_connection_impl> m_pImpl;
    };
}
