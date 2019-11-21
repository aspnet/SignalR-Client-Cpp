// Copyright (c) .NET Foundation. All rights reserved.
// Licensed under the Apache License, Version 2.0. See License.txt in the project root for license information.

#include "stdafx.h"
#include "json_helpers.h"
#include <cmath>

namespace signalr
{
    signalr::value createValue(const Json::Value& v)
    {
        switch (v.type())
        {
        case Json::ValueType::booleanValue:
            return signalr::value(v.asBool());
        case Json::ValueType::realValue:
        case Json::ValueType::intValue:
        case Json::ValueType::uintValue:
            return signalr::value(v.asDouble());
        case Json::ValueType::stringValue:
            return signalr::value(v.asString());
        case Json::ValueType::arrayValue:
        {
            std::vector<signalr::value> vec;
            for (auto& val : v)
            {
                vec.push_back(createValue(val));
            }
            return signalr::value(std::move(vec));
        }
        case Json::ValueType::objectValue:
        {
            std::map<std::string, signalr::value> map;
            for (const auto& val : v.getMemberNames())
            {
                map.insert({ val, createValue(v[val]) });
            }
            return signalr::value(std::move(map));
        }
        case Json::ValueType::nullValue:
        default:
            return signalr::value();
        }
    }

    Json::Value createJson(const signalr::value& v)
    {
        switch (v.type())
        {
        case signalr::value_type::boolean:
            return Json::Value(v.as_bool());
        case signalr::value_type::float64:
        {
            auto value = v.as_double();
            double intPart;
            // REVIEW: workaround for 1.0 being output as 1.0 instead of 1
            if (std::modf(value, &intPart) == 0)
            {
                return Json::Value((int)intPart);
            }
            return Json::Value(v.as_double());
        }
        case signalr::value_type::string:
            return Json::Value(v.as_string());
        case signalr::value_type::array:
        {
            const auto& array = v.as_array();
            Json::Value vec(Json::ValueType::arrayValue);
            for (auto& val : array)
            {
                vec.append(createJson(val));
            }
            return vec;
        }
        case signalr::value_type::map:
        {
            const auto& obj = v.as_map();
            Json::Value object(Json::ValueType::objectValue);
            for (auto& val : obj)
            {
                object[val.first] = createJson(val.second);
            }
            return object;
        }
        case signalr::value_type::null:
        default:
            return Json::Value(Json::ValueType::nullValue);
        }
    }

    Json::StreamWriterBuilder getJsonWriter()
    {
        auto writer = Json::StreamWriterBuilder();
        writer["commentStyle"] = "None";
        writer["indentation"] = "";
        return writer;
    }
}