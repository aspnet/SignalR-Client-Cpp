// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#include "stdafx.h"
#include "signalrclient/connection.h"
#include "signalrclient/transport_type.h"
#include "connection_impl.h"
#include "completion_event.h"

namespace signalr
{
    connection::connection(const std::string& url, trace_level trace_level, std::shared_ptr<log_writer> log_writer,
        std::function<std::shared_ptr<http_client>(const signalr_client_config&)> http_client_factory,
        std::function<std::shared_ptr<websocket_client>(const signalr_client_config&)> websocket_factory, bool skip_negotiation)
        : m_pImpl(connection_impl::create(url, trace_level, std::move(log_writer), http_client_factory, websocket_factory, skip_negotiation))
    {}

    // Do NOT remove this destructor. Letting the compiler generate and inline the default dtor may lead to
    // undefinded behavior since we are using an incomplete type. More details here:  http://herbsutter.com/gotw/_100/
    connection::~connection()
    {
        completion_event completion;
        m_pImpl->stop([completion](std::exception_ptr) mutable
            {
                completion.set();
            }, nullptr);
        completion.get();
    }

    void connection::start(std::function<void(std::exception_ptr)> callback) noexcept
    {
        m_pImpl->start(callback);
    }

    void connection::send(const std::string& data, transfer_format transfer_format, std::function<void(std::exception_ptr)> callback) noexcept
    {
        m_pImpl->send(data, transfer_format, callback);
    }

    void connection::set_message_received(const message_received_handler& message_received_callback)
    {
        m_pImpl->set_message_received(message_received_callback);
    }

    void connection::set_disconnected(const std::function<void(std::exception_ptr)>& disconnected_callback)
    {
        m_pImpl->set_disconnected(disconnected_callback);
    }

    void connection::set_client_config(const signalr_client_config& config)
    {
        m_pImpl->set_client_config(config);
    }

    void connection::stop(std::function<void(std::exception_ptr)> callback, std::exception_ptr exception) noexcept
    {
        m_pImpl->stop(callback, exception);
    }

    connection_state connection::get_connection_state() const noexcept
    {
        return m_pImpl->get_connection_state();
    }

    std::string connection::get_connection_id() const
    {
        return m_pImpl->get_connection_id();
    }
}
