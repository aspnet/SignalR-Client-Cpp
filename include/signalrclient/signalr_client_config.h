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
    class signalr_client_config final
    {
    public:
        SIGNALRCLIENT_API signalr_client_config();

        SIGNALRCLIENT_API const std::map<std::string, std::string>& get_http_headers() const noexcept;
        SIGNALRCLIENT_API void set_http_headers(std::map<std::string, std::string> http_headers) noexcept;
        SIGNALRCLIENT_API void set_scheduler(std::shared_ptr<scheduler> scheduler) noexcept;
        SIGNALRCLIENT_API std::shared_ptr<scheduler> get_scheduler() const noexcept;
        SIGNALRCLIENT_API void set_handshake_timeout(std::chrono::milliseconds);
        SIGNALRCLIENT_API std::chrono::milliseconds get_handshake_timeout() const noexcept;
        SIGNALRCLIENT_API void set_server_timeout(std::chrono::milliseconds);
        SIGNALRCLIENT_API std::chrono::milliseconds get_server_timeout() const noexcept;
        SIGNALRCLIENT_API void set_keepalive_interval(std::chrono::milliseconds);
        SIGNALRCLIENT_API std::chrono::milliseconds get_keepalive_interval() const noexcept;

    private:
        std::map<std::string, std::string> m_http_headers;
        std::shared_ptr<scheduler> m_scheduler;
        std::chrono::milliseconds m_handshake_timeout;
        std::chrono::milliseconds m_server_timeout;
        std::chrono::milliseconds m_keepalive_interval;
    };
}
