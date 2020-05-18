// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#pragma once

#include <unordered_map>
#include "connection_impl.h"
#include "callback_manager.h"
#include "case_insensitive_comparison_utils.h"
#include "completion_event.h"
#include "signalrclient/signalr_value.h"
#include "hub_protocol.h"

namespace signalr
{
    class websocket_client;

    // Note:
    // Factory methods and private constructors prevent from using this class incorrectly. Because this class
    // derives from `std::enable_shared_from_this` the instance has to be owned by a `std::shared_ptr` whenever
    // a member method calls `std::shared_from_this()` otherwise the behavior is undefined. Therefore constructors
    // are private to disallow creating instances directly and factory methods return `std::shared_ptr<connection_impl>`.
    class hub_connection_impl : public std::enable_shared_from_this<hub_connection_impl>
    {
    public:
        static std::shared_ptr<hub_connection_impl> create(const std::string& url, trace_level trace_level,
            const std::shared_ptr<log_writer>& log_writer, std::shared_ptr<http_client> http_client,
            std::function<std::shared_ptr<websocket_client>(const signalr_client_config&)> websocket_factory, bool skip_negotiation);

        hub_connection_impl(const hub_connection_impl&) = delete;
        hub_connection_impl& operator=(const hub_connection_impl&) = delete;

        void on(const std::string& event_name, const std::function<void(const signalr::value &)>& handler);

        void invoke(const std::string& method_name, const signalr::value& arguments, std::function<void(const signalr::value&, std::exception_ptr)> callback) noexcept;
        void send(const std::string& method_name, const signalr::value& arguments, std::function<void(std::exception_ptr)> callback) noexcept;

        void start(std::function<void(std::exception_ptr)> callback) noexcept;
        void stop(std::function<void(std::exception_ptr)> callback) noexcept;

        connection_state get_connection_state() const noexcept;
        std::string get_connection_id() const;

        void set_client_config(const signalr_client_config& config);
        void set_disconnected(const std::function<void()>& disconnected);

    private:
        hub_connection_impl(const std::string& url, trace_level trace_level,
            const std::shared_ptr<log_writer>& log_writer, std::shared_ptr<http_client> http_client,
            std::function<std::shared_ptr<websocket_client>(const signalr_client_config&)> websocket_factory,
            bool skip_negotiation);

        std::shared_ptr<connection_impl> m_connection;
        logger m_logger;
        callback_manager m_callback_manager;
        std::unordered_map<std::string, std::function<void(const signalr::value&)>, case_insensitive_hash, case_insensitive_equals> m_subscriptions;
        bool m_handshakeReceived;
        std::shared_ptr<completion_event> m_handshakeTask;
        std::function<void()> m_disconnected;
        signalr_client_config m_signalr_client_config;
        std::shared_ptr<hub_protocol> m_protocol;

        std::mutex m_stop_callback_lock;
        std::vector<std::function<void(std::exception_ptr)>> m_stop_callbacks;

        void initialize();

        void process_message(std::string&& message);

        void invoke_hub_method(const std::string& method_name, const signalr::value& arguments, const std::string& callback_id,
            std::function<void()> set_completion, std::function<void(const std::exception_ptr)> set_exception) noexcept;
        bool invoke_callback(const signalr::value& message);
    };
}
