// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#pragma once

#ifdef USE_CPPRESTSDK
#pragma warning (push)
#pragma warning (disable : 5204 4355 4625 4626 4868)
#include <cpprest/http_client.h>
#include <cpprest/ws_client.h>
#pragma warning (pop)
#endif

#include "_exports.h"
#include <map>
#include <string>
#include "scheduler.h"
#include <memory>

namespace signalr
{
    class signalr_client_config
    {
    public:
#ifdef USE_CPPRESTSDK
        SIGNALRCLIENT_API void __cdecl set_proxy(const web::web_proxy &proxy);
        // Please note that setting credentials does not work in all cases.
        // For example, Basic Authentication fails under Win32.
        // As a workaround, you can set the required authorization headers directly
        // using signalr_client_config::set_http_headers
        SIGNALRCLIENT_API void __cdecl set_credentials(const web::credentials &credentials);

        SIGNALRCLIENT_API web::http::client::http_client_config __cdecl get_http_client_config() const;
        SIGNALRCLIENT_API void __cdecl set_http_client_config(const web::http::client::http_client_config& http_client_config);

        SIGNALRCLIENT_API web::websockets::client::websocket_client_config __cdecl get_websocket_client_config() const noexcept;
        SIGNALRCLIENT_API void __cdecl set_websocket_client_config(const web::websockets::client::websocket_client_config& websocket_client_config);
#endif

        SIGNALRCLIENT_API signalr_client_config();

        SIGNALRCLIENT_API const std::map<std::string, std::string>& __cdecl get_http_headers() const noexcept;
        SIGNALRCLIENT_API std::map<std::string, std::string>& __cdecl get_http_headers() noexcept;
        SIGNALRCLIENT_API void __cdecl set_http_headers(const std::map<std::string, std::string>& http_headers);
        SIGNALRCLIENT_API void __cdecl set_scheduler(std::shared_ptr<scheduler> scheduler);
        SIGNALRCLIENT_API const std::shared_ptr<scheduler>& __cdecl get_scheduler() const noexcept;
        SIGNALRCLIENT_API void set_handshake_timeout(std::chrono::milliseconds);
        SIGNALRCLIENT_API std::chrono::milliseconds get_handshake_timeout() const noexcept;
        SIGNALRCLIENT_API void set_server_timeout(std::chrono::milliseconds);
        SIGNALRCLIENT_API std::chrono::milliseconds get_server_timeout() const noexcept;
        SIGNALRCLIENT_API void set_keepalive_interval(std::chrono::milliseconds);
        SIGNALRCLIENT_API std::chrono::milliseconds get_keepalive_interval() const noexcept;

    private:
#ifdef USE_CPPRESTSDK
        web::http::client::http_client_config m_http_client_config;
        web::websockets::client::websocket_client_config m_websocket_client_config;
#endif
        std::map<std::string, std::string> m_http_headers;
        std::shared_ptr<scheduler> m_scheduler;
        std::chrono::milliseconds m_handshake_timeout;
        std::chrono::milliseconds m_server_timeout;
        std::chrono::milliseconds m_keepalive_interval;
    };
}
