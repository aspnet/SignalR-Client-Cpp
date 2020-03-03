// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#pragma once

#ifdef USE_CPPRESTSDK
#include "cpprest/http_client.h"
#include "cpprest/ws_client.h"
#endif

#include "_exports.h"
#include <map>
#include <string>

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

        SIGNALRCLIENT_API const std::map<std::string, std::string>& __cdecl get_http_headers() const noexcept;
        SIGNALRCLIENT_API std::map<std::string, std::string>& __cdecl get_http_headers() noexcept;
        SIGNALRCLIENT_API void __cdecl set_http_headers(const std::map<std::string, std::string>& http_headers);

    private:
#ifdef USE_CPPRESTSDK
        web::http::client::http_client_config m_http_client_config;
        web::websockets::client::websocket_client_config m_websocket_client_config;
#endif
        std::map<std::string, std::string> m_http_headers;
    };
}
