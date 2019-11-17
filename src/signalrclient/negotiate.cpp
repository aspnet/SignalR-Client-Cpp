// Copyright (c) .NET Foundation. All rights reserved.
// Licensed under the Apache License, Version 2.0. See License.txt in the project root for license information.

#include "stdafx.h"
#include "negotiate.h"
#include "url_builder.h"
#include "signalrclient/signalr_exception.h"

namespace signalr
{
    namespace negotiate
    {
        const int negotiate_version = 1;

        void negotiate(http_client& client, const std::string& base_url,
            const signalr_client_config& config,
            std::function<void(negotiation_response&&, std::exception_ptr)> callback) noexcept
        {
            std::string negotiate_url;
            try
            {
                negotiate_url = url_builder::build_negotiate(base_url);
                negotiate_url = url_builder::add_query_string(negotiate_url, "negotiateVersion=" + std::to_string(negotiate_version));
            }
            catch (...)
            {
                callback({}, std::current_exception());
                return;
            }

            // TODO: signalr_client_config
            http_request request;
            request.method = http_method::POST;

            for (auto& header : config.get_http_headers())
            {
                request.headers.insert(std::make_pair(utility::conversions::to_utf8string(header.first), utility::conversions::to_utf8string(header.second)));
            }

            client.send(negotiate_url, request, [callback](const http_response& http_response, std::exception_ptr exception)
            {
                if (exception != nullptr)
                {
                    callback({}, exception);
                    return;
                }

                if (http_response.status_code != 200)
                {
                    callback({}, std::make_exception_ptr(
                        signalr_exception("negotiate failed with status code " + std::to_string(http_response.status_code))));
                    return;
                }

                try
                {
                    auto negotiation_response_json = web::json::value::parse(utility::conversions::to_string_t(http_response.content));

                    negotiation_response response;

                    if (negotiation_response_json.has_field(_XPLATSTR("error")))
                    {
                        response.error = utility::conversions::to_utf8string(negotiation_response_json[_XPLATSTR("error")].as_string());
                        callback(std::move(response), nullptr);
                        return;
                    }

                    int server_negotiate_version = 0;
                    if (negotiation_response_json.has_field(_XPLATSTR("negotiateVersion")))
                    {
                        server_negotiate_version = negotiation_response_json[_XPLATSTR("negotiateVersion")].as_integer();
                    }

                    if (negotiation_response_json.has_field(_XPLATSTR("connectionId")))
                    {
                        response.connectionId = utility::conversions::to_utf8string(negotiation_response_json[_XPLATSTR("connectionId")].as_string());
                    }

                    if (negotiation_response_json.has_field(_XPLATSTR("connectionToken")))
                    {
                        response.connectionToken = utility::conversions::to_utf8string(negotiation_response_json[_XPLATSTR("connectionToken")].as_string());
                    }

                    if (server_negotiate_version <= 0)
                    {
                        response.connectionToken = response.connectionId;
                    }

                    if (negotiation_response_json.has_field(_XPLATSTR("availableTransports")))
                    {
                        for (auto transportData : negotiation_response_json[_XPLATSTR("availableTransports")].as_array())
                        {
                            available_transport transport;
                            transport.transport = utility::conversions::to_utf8string(transportData[_XPLATSTR("transport")].as_string());

                            for (auto format : transportData[_XPLATSTR("transferFormats")].as_array())
                            {
                                transport.transfer_formats.push_back(utility::conversions::to_utf8string(format.as_string()));
                            }

                            response.availableTransports.push_back(transport);
                        }
                    }

                    if (negotiation_response_json.has_field(_XPLATSTR("url")))
                    {
                        response.url = utility::conversions::to_utf8string(negotiation_response_json[_XPLATSTR("url")].as_string());

                        if (negotiation_response_json.has_field(_XPLATSTR("accessToken")))
                        {
                            response.accessToken = utility::conversions::to_utf8string(negotiation_response_json[_XPLATSTR("accessToken")].as_string());
                        }
                    }

                    if (negotiation_response_json.has_field(_XPLATSTR("ProtocolVersion")))
                    {
                        callback({}, std::make_exception_ptr(
                            signalr_exception("Detected a connection attempt to an ASP.NET SignalR Server. This client only supports connecting to an ASP.NET Core SignalR Server. See https://aka.ms/signalr-core-differences for details.")));
                        return;
                    }

                    callback(std::move(response), nullptr);
                }
                catch (...)
                {
                    callback({}, std::current_exception());
                }
            });
        }
    }
}
