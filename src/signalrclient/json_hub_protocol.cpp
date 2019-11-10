// Copyright (c) .NET Foundation. All rights reserved.
// Licensed under the Apache License, Version 2.0. See License.txt in the project root for license information.

#include "stdafx.h"
#include "json_hub_protocol.h"
#include <cpprest\json.h>

namespace signalr
{
    signalr::value createValue(const web::json::value& v)
    {
        switch (v.type())
        {
        case web::json::value::Boolean:
            return signalr::value(v.as_bool());
        case web::json::value::Number:
            return signalr::value(v.as_double());
        case web::json::value::String:
            return signalr::value(utility::conversions::to_utf8string(v.as_string()));
        case web::json::value::Array:
        {
            auto& array = v.as_array();
            std::vector<signalr::value> vec;
            for (auto& val : array)
            {
                vec.push_back(createValue(val));
            }
            return signalr::value(std::move(vec));
        }
        case web::json::value::Object:
        {
            auto& obj = v.as_object();
            std::map<std::string, signalr::value> map;
            for (auto& val : obj)
            {
                map.insert({ utility::conversions::to_utf8string(val.first), createValue(val.second) });
            }
            return signalr::value(std::move(map));
        }
        case web::json::value::Null:
        default:
            return signalr::value();
        }
    }

    web::json::value createJson(const signalr::value& v)
    {
        switch (v.type())
        {
        case signalr::value_type::boolean:
            return web::json::value(v.as_bool());
        case signalr::value_type::float64:
            return web::json::value(v.as_double());
        case signalr::value_type::string:
            return web::json::value(utility::conversions::to_string_t(v.as_string()));
        case signalr::value_type::array:
        {
            const auto& array = v.as_array();
            auto vec = std::vector<web::json::value>();
            for (auto& val : array)
            {
                vec.push_back(createJson(val));
            }
            return web::json::value::array(vec);
        }
        case signalr::value_type::map:
        {
            const auto& obj = v.as_map();
            auto o = web::json::value::object();
            for (auto& val : obj)
            {
                o[utility::conversions::to_string_t(val.first)] = createJson(val.second);
            }
            return o;
        }
        case signalr::value_type::null:
        default:
            return web::json::value::null();
        }
    }

    std::string signalr::json_hub_protocol::write_message(const signalr::value& hub_message)
    {
        return utility::conversions::to_utf8string(createJson(hub_message).serialize()) + '\x1e';
    }

    signalr::value json_hub_protocol::parse_message(const std::string& message)
    {
        const auto result = web::json::value::parse(utility::conversions::to_string_t(message));
        return createValue(result);
    }
}