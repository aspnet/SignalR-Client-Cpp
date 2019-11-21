// Copyright (c) .NET Foundation. All rights reserved.
// Licensed under the Apache License, Version 2.0. See License.txt in the project root for license information.

#pragma once

#include "signalrclient/signalr_value.h"
#include <json/json.h>

namespace signalr
{
    static constexpr char record_separator = '\x1e';

    signalr::value createValue(const Json::Value& v);

    Json::Value createJson(const signalr::value& v);

    Json::StreamWriterBuilder getJsonWriter();
}