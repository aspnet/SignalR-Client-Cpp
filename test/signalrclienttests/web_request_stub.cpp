// Copyright (c) .NET Foundation. All rights reserved.
// Licensed under the Apache License, Version 2.0. See License.txt in the project root for license information.

#include "stdafx.h"
#include "web_request_stub.h"

web_request_stub::web_request_stub(unsigned short status_code, const std::string& reason_phrase, const std::string& response_body)
    : web_request(""), m_status_code(status_code), m_reason_phrase(reason_phrase), m_response_body(response_body)
{ }

void web_request_stub::set_method(const std::string &method)
{
    m_method = method;
}

void web_request_stub::set_user_agent(const std::string &user_agent_string)
{
    m_user_agent_string = user_agent_string;
}

void web_request_stub::set_client_config(const signalr_client_config& config)
{
    m_signalr_client_config = config;
}

pplx::task<web_response> web_request_stub::get_response()
{
    on_get_response(*this);

    return pplx::task_from_result<web_response>(
        web_response{ m_status_code, m_reason_phrase, pplx::task_from_result<std::string>(m_response_body) });
}
