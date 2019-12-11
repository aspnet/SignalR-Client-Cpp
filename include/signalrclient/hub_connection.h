// Copyright (c) .NET Foundation. All rights reserved.
// Licensed under the Apache License, Version 2.0. See License.txt in the project root for license information.

#pragma once

#include "_exports.h"
#include <memory>
#include <functional>
#include "connection_state.h"
#include "trace_level.h"
#include "log_writer.h"
#include "signalr_client_config.h"
#include "signalr_value.h"

namespace signalr
{
    class http_client;
    class websocket_client;
    class hub_connection_impl;
    class hub_connection_builder;

    class hub_connection
    {
    public:
        typedef std::function<void __cdecl (const signalr::value&)> method_invoked_handler;

        SIGNALRCLIENT_API ~hub_connection();

        hub_connection(const hub_connection&) = delete;

        hub_connection& operator=(const hub_connection&) = delete;

        SIGNALRCLIENT_API hub_connection(hub_connection&&) noexcept;

        SIGNALRCLIENT_API hub_connection& operator=(hub_connection&&) noexcept;

        SIGNALRCLIENT_API void __cdecl start(std::function<void(std::exception_ptr)> callback) noexcept;
        SIGNALRCLIENT_API void __cdecl stop(std::function<void(std::exception_ptr)> callback) noexcept;

        SIGNALRCLIENT_API connection_state __cdecl get_connection_state() const;
        SIGNALRCLIENT_API std::string __cdecl get_connection_id() const;

        SIGNALRCLIENT_API void __cdecl set_disconnected(const std::function<void __cdecl()>& disconnected_callback);

        SIGNALRCLIENT_API void __cdecl set_client_config(const signalr_client_config& config);

        SIGNALRCLIENT_API void __cdecl on(const std::string& event_name, const method_invoked_handler& handler);

        SIGNALRCLIENT_API void invoke(const std::string& method_name, const signalr::value& arguments = signalr::value(), std::function<void(const signalr::value&, std::exception_ptr)> callback = [](const signalr::value&, std::exception_ptr) {}) noexcept;

        SIGNALRCLIENT_API void send(const std::string& method_name, const signalr::value& arguments = signalr::value(), std::function<void(std::exception_ptr)> callback = [](std::exception_ptr) {}) noexcept;

    private:
        friend class hub_connection_builder;

        explicit hub_connection(const std::string& url, trace_level trace_level = trace_level::all,
            std::shared_ptr<log_writer> log_writer = nullptr, std::shared_ptr<http_client> http_client = nullptr,
            std::function<std::shared_ptr<websocket_client>()> websocket_factory = nullptr);

        std::shared_ptr<hub_connection_impl> m_pImpl;
    };
}
