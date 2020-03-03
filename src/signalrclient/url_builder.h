// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#pragma once

#include "signalrclient/transport_type.h"

namespace signalr
{
    namespace url_builder
    {
        std::string build_negotiate(const std::string& base_url);
        std::string build_connect(const std::string& base_url, transport_type transport, const std::string& query_string);
        std::string add_query_string(const std::string& base_url, const std::string& query_string);
    }
}
