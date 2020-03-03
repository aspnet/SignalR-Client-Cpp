// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#pragma once

#include "signalrclient/signalr_value.h"
#include <json/json.h>
#include <memory>

namespace signalr
{
    static constexpr char record_separator = '\x1e';

    signalr::value createValue(const Json::Value& v);

    Json::Value createJson(const signalr::value& v);

    Json::StreamWriterBuilder getJsonWriter();
    std::unique_ptr<Json::CharReader> getJsonReader();
}