// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#include "stdafx.h"
#include "negotiate.h"
#include "url_builder.h"
#include "signalrclient/signalr_exception.h"
#include "json_helpers.h"
#include "cancellation_token_source.h"

namespace signalr
{
    namespace negotiate
    {
        const int negotiate_version = 1;

        void negotiate(std::shared_ptr<http_client> client, const std::string& base_url,
            const signalr_client_config& config,
            std::function<void(negotiation_response&&, std::exception_ptr)> callback, cancellation_token token) noexcept
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
            request.headers = config.get_http_headers();
#ifdef USE_CPPRESTSDK
            request.timeout = config.get_http_client_config().timeout();
#endif

            client->send(negotiate_url, request, [callback, token](const http_response& http_response, std::exception_ptr exception)
            {
                if (exception != nullptr)
                {
                    callback({}, exception);
                    return;
                }

                if (token.is_canceled())
                {
                    callback({}, std::make_exception_ptr(canceled_exception()));
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
                    Json::Value negotiation_response_json;
                    auto reader = getJsonReader();
                    std::string errors;

                    if (!reader->parse(http_response.content.c_str(), http_response.content.c_str() + http_response.content.size(), &negotiation_response_json, &errors))
                    {
                        throw signalr_exception(errors);
                    }

                    negotiation_response response;

                    if (negotiation_response_json.isMember("error"))
                    {
                        response.error = negotiation_response_json["error"].asString();
                        callback(std::move(response), nullptr);
                        return;
                    }

                    int server_negotiate_version = 0;
                    if (negotiation_response_json.isMember("negotiateVersion"))
                    {
                        server_negotiate_version = negotiation_response_json["negotiateVersion"].asInt();
                    }

                    if (negotiation_response_json.isMember("connectionId"))
                    {
                        response.connectionId = negotiation_response_json["connectionId"].asString();
                    }

                    if (negotiation_response_json.isMember("connectionToken"))
                    {
                        response.connectionToken = negotiation_response_json["connectionToken"].asString();
                    }

                    if (server_negotiate_version <= 0)
                    {
                        response.connectionToken = response.connectionId;
                    }

                    if (negotiation_response_json.isMember("availableTransports"))
                    {
                        for (const auto& transportData : negotiation_response_json["availableTransports"])
                        {
                            available_transport transport;
                            transport.transport = transportData["transport"].asString();

                            for (const auto& format : transportData["transferFormats"])
                            {
                                transport.transfer_formats.push_back(format.asString());
                            }

                            response.availableTransports.push_back(transport);
                        }
                    }

                    if (negotiation_response_json.isMember("url"))
                    {
                        response.url = negotiation_response_json["url"].asString();

                        if (negotiation_response_json.isMember("accessToken"))
                        {
                            response.accessToken = negotiation_response_json["accessToken"].asString();
                        }
                    }

                    if (negotiation_response_json.isMember("ProtocolVersion"))
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
            }, token);
        }
    }
}
