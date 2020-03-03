// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#include "stdafx.h"
#include "uri_builder.h"
#include "signalrclient/transport_type.h"

namespace signalr
{
    namespace url_builder
    {
        uri_builder &convert_to_websocket_url(uri_builder &builder, transport_type transport)
        {
            if (transport == transport_type::websockets)
            {
                if (builder.scheme() == "https")
                {
                    builder.set_scheme("wss");
                }
                else
                {
                    builder.set_scheme("ws");
                }
            }

            return builder;
        }

        uri_builder build_uri(const std::string& base_url, const std::string& command, const std::string& query_string)
        {
            uri_builder builder(base_url);
            builder.append_path(command);
            return builder.append_query(query_string);
        }

        uri_builder build_uri(const std::string& base_url, const std::string& command)
        {
            uri_builder builder(base_url);
            return builder.append_path(command);
        }

        std::string build_negotiate(const std::string& base_url)
        {
            return build_uri(base_url, "negotiate").to_string();
        }

        std::string build_connect(const std::string& base_url, transport_type transport, const std::string& query_string)
        {
            auto builder = build_uri(base_url, "", query_string);
            return convert_to_websocket_url(builder, transport).to_string();
        }

        std::string add_query_string(const std::string& base_url, const std::string& query_string)
        {
            uri_builder builder(base_url);
            return builder.append_query(query_string).to_string();
        }
    }
}
