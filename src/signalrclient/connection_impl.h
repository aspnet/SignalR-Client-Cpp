// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#pragma once

#include <mutex>
#include <atomic>
#include "signalrclient/http_client.h"
#include "signalrclient/trace_level.h"
#include "signalrclient/connection_state.h"
#include "signalrclient/signalr_client_config.h"
#include "transport_factory.h"
#include "logger.h"
#include "negotiation_response.h"
#include "cancellation_token.h"

namespace signalr
{
    class websocket_client;

    // Note:
    // Factory methods and private constructors prevent from using this class incorrectly. Because this class
    // derives from `std::enable_shared_from_this` the instance has to be owned by a `std::shared_ptr` whenever
    // a member method calls `std::shared_from_this()` otherwise the behavior is undefined. Therefore constructors
    // are private to disallow creating instances directly and factory methods return `std::shared_ptr<connection_impl>`.
    class connection_impl : public std::enable_shared_from_this<connection_impl>
    {
    public:
        static std::shared_ptr<connection_impl> create(const std::string& url, trace_level trace_level, const std::shared_ptr<log_writer>& log_writer);

        static std::shared_ptr<connection_impl> create(const std::string& url, trace_level trace_level, const std::shared_ptr<log_writer>& log_writer,
            std::shared_ptr<http_client> http_client, std::function<std::shared_ptr<websocket_client>(const signalr_client_config&)> websocket_factory,
            bool skip_negotiation);

        connection_impl(const connection_impl&) = delete;

        connection_impl& operator=(const connection_impl&) = delete;

        ~connection_impl();

        void start(std::function<void(std::exception_ptr)> callback) noexcept;
        void send(const std::string &data, std::function<void(std::exception_ptr)> callback) noexcept;
        void stop(std::function<void(std::exception_ptr)> callback) noexcept;

        connection_state get_connection_state() const noexcept;
        std::string get_connection_id() const noexcept;

        void set_message_received(const std::function<void(std::string&&)>& message_received);
        void set_disconnected(const std::function<void()>& disconnected);
        void set_client_config(const signalr_client_config& config);

    private:
        std::string m_base_url;
        std::atomic<connection_state> m_connection_state;
        logger m_logger;
        std::shared_ptr<transport> m_transport;
        std::unique_ptr<transport_factory> m_transport_factory;
        bool m_skip_negotiation;

        std::function<void(std::string&&)> m_message_received;
        std::function<void()> m_disconnected;
        signalr_client_config m_signalr_client_config;

        std::shared_ptr<cancellation_token> m_disconnect_cts;
        std::mutex m_stop_lock;
        cancellation_token m_start_completed_event;
        std::string m_connection_id;
        std::string m_connection_token;
        std::shared_ptr<http_client> m_http_client;

        connection_impl(const std::string& url, trace_level trace_level, const std::shared_ptr<log_writer>& log_writer,
            std::unique_ptr<http_client> http_client, std::unique_ptr<transport_factory> transport_factory, bool skip_negotiation);

        connection_impl(const std::string& url, trace_level trace_level, const std::shared_ptr<log_writer>& log_writer,
            std::shared_ptr<http_client> http_client, std::function<std::shared_ptr<websocket_client>(const signalr_client_config&)> websocket_factory, bool skip_negotiation);

        void start_transport(const std::string& url, std::function<void(std::shared_ptr<transport>, std::exception_ptr)> callback);
        void send_connect_request(const std::shared_ptr<transport>& transport,
            const std::string& url, std::function<void(std::exception_ptr)> callback);
        void start_negotiate(const std::string& url, int redirect_count, std::function<void(std::exception_ptr)> callback);

        void process_response(std::string&& response);

        void shutdown(std::function<void(std::exception_ptr)> callback);
        void stop_connection(std::exception_ptr);

        bool change_state(connection_state old_state, connection_state new_state);
        connection_state change_state(connection_state new_state);
        void handle_connection_state_change(connection_state old_state, connection_state new_state);
        void invoke_message_received(std::string&& message);

        static std::string translate_connection_state(connection_state state);
        void ensure_disconnected(const std::string& error_message) const;
    };
}
