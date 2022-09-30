// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#include "stdafx.h"
#include "json_helpers.h"
#include <cmath>
#include <stdint.h>

namespace signalr
{
    char record_separator = '\x1e';

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

    char getBase64Value(uint32_t i)
    {
        char index = (char)i;
        if (index < 26)
        {
            return 'A' + index;
        }
        if (index < 52)
        {
            return 'a' + (index - 26);
        }
        if (index < 62)
        {
            return '0' + (index - 52);
        }
        if (index == 62)
        {
            return '+';
        }
        if (index == 63)
        {
            return '/';
        }

        throw std::out_of_range("base64 table index out of range: " + std::to_string(index));
    }

    std::string base64Encode(const std::vector<uint8_t>& data)
    {
        std::string base64result;

        size_t i = 0;
        while (i <= data.size() - 3)
        {
            uint32_t b = ((uint32_t)data[i] << 16) | ((uint32_t)data[i + 1] << 8) | (uint32_t)data[i + 2];
            base64result.push_back(getBase64Value((b >> 18) & 0x3F));
            base64result.push_back(getBase64Value((b >> 12) & 0x3F));
            base64result.push_back(getBase64Value((b >> 6) & 0x3F));
            base64result.push_back(getBase64Value(b & 0x3F));

            i += 3;
        }
        if (data.size() - i == 2)
        {
            uint32_t b = ((uint32_t)data[i] << 8) | (uint32_t)data[i + 1];
            base64result.push_back(getBase64Value((b >> 10) & 0x3F));
            base64result.push_back(getBase64Value((b >> 4) & 0x3F));
            base64result.push_back(getBase64Value((b << 2) & 0x3F));
            base64result.push_back('=');
        }
        else if (data.size() - i == 1)
        {
            uint32_t b = (uint32_t)data[i];
            base64result.push_back(getBase64Value((b >> 2) & 0x3F));
            base64result.push_back(getBase64Value((b << 4) & 0x3F));
            base64result.push_back('=');
            base64result.push_back('=');
        }

        return base64result;
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
            // Workaround for 1.0 being output as 1.0 instead of 1
            // because the server expects certain values to be 1 instead of 1.0 (like protocol version)
            if (std::modf(value, &intPart) == 0)
            {
                if (value < 0)
                {
                    if (value >= (double)INT64_MIN)
                    {
                        // Fits within int64_t
                        return Json::Value(static_cast<int64_t>(intPart));
                    }
                    else
                    {
                        // Remain as double
                        return Json::Value(value);
                    }
                }
                else
                {
                    if (value <= (double)UINT64_MAX)
                    {
                        // Fits within uint64_t
                        return Json::Value(static_cast<uint64_t>(intPart));
                    }
                    else
                    {
                        // Remain as double
                        return Json::Value(value);
                    }
                }
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
        case signalr::value_type::binary:
        {
            const auto& binary = v.as_binary();
            return Json::Value(base64Encode(binary));
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

    std::unique_ptr<Json::CharReader> getJsonReader()
    {
        auto builder = Json::CharReaderBuilder();
        Json::CharReaderBuilder::strictMode(&builder.settings_);
        std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
        return reader;
    }
}