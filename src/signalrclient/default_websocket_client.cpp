// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#include "stdafx.h"

#ifdef USE_CPPRESTSDK
#include "default_websocket_client.h"
#include <cpprest/producerconsumerstream.h>

namespace signalr
{
    namespace
    {
        static web::websockets::client::websocket_client_config create_client_config(const signalr_client_config& signalr_client_config) noexcept
        {
            auto websocket_client_config = signalr_client_config.get_websocket_client_config();
            auto& websocket_headers = websocket_client_config.headers();
            for (auto& header : signalr_client_config.get_http_headers())
            {
                websocket_headers.add(utility::conversions::to_string_t(header.first), utility::conversions::to_string_t(header.second));
            }

            return websocket_client_config;
        }
    }

    default_websocket_client::default_websocket_client(const signalr_client_config& signalr_client_config) noexcept
        : m_underlying_client(create_client_config(signalr_client_config))
    { }

    void default_websocket_client::start(const std::string& url, std::function<void(std::exception_ptr)> callback)
    {
        m_underlying_client.connect(utility::conversions::to_string_t(url))
            .then([callback](pplx::task<void> task)
            {
                try
                {
                    task.get();
                    callback(nullptr);
                }
                catch (...)
                {
                    callback(std::current_exception());
                }
            });
    }

    void default_websocket_client::stop(std::function<void(std::exception_ptr)> callback)
    {
        m_underlying_client.close()
            .then([callback](pplx::task<void> task)
            {
                try
                {
                    task.get();
                    callback(nullptr);
                }
                catch (...)
                {
                    callback(std::current_exception());
                }
            });
    }

    void default_websocket_client::send(const std::string& payload, signalr::transfer_format transfer_format, std::function<void(std::exception_ptr)> callback)
    {
        web::websockets::client::websocket_outgoing_message msg;

        if (transfer_format == signalr::transfer_format::binary)
        {
            concurrency::streams::producer_consumer_buffer<uint8_t> b;
            b.putn_nocopy(reinterpret_cast<const uint8_t*>(payload.data()), payload.length());
            msg.set_binary_message(b.create_istream(), payload.length());
        }
        else
        {
            msg.set_utf8_message(payload);
        }
        m_underlying_client.send(msg)
            .then([callback](pplx::task<void> task)
            {
                try
                {
                    task.get();
                    callback(nullptr);
                }
                catch (...)
                {
                    callback(std::current_exception());
                }
            });
    }

    void default_websocket_client::receive(std::function<void(const std::string&, std::exception_ptr)> callback)
    {
        m_underlying_client.receive()
            .then([callback](pplx::task<web::websockets::client::websocket_incoming_message> task)
            {
                try
                {
                    auto response = task.get();
                    if (response.message_type() == web::websockets::client::websocket_message_type::binary_message)
                    {
                        concurrency::streams::producer_consumer_buffer<uint8_t> b;
                        response.body().read_to_end(b).get();
                        auto t = b.create_ostream().streambuf().in_avail();
                        std::string msg(t, ' ');
                        b.create_istream().streambuf().getn(reinterpret_cast<uint8_t*>(&msg[0]), t);

                        callback(msg, nullptr);
                    }
                    else
                    {
                        callback(response.extract_string().get(), nullptr);
                    }
                }
                catch (...)
                {
                    callback("", std::current_exception());
                }
            });
    }
}
#endif