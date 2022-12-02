// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#include "stdafx.h"
#include "signalrclient/signalr_value.h"
#include "signalrclient/signalr_exception.h"
#include <string>

namespace signalr
{
    std::string value_type_to_string(enum value::type v)
    {
        switch (v)
        {
        case signalr::value::type::map:
            return "map";
        case signalr::value::type::array:
            return "array";
        case signalr::value::type::string:
            return "string";
        case signalr::value::type::float64:
            return "float64";
        case signalr::value::type::null:
            return "null";
        case signalr::value::type::boolean:
            return "boolean";
        case signalr::value::type::binary:
            return "binary";
        default:
            return std::to_string((int)v);
        }
    }

    value::value() : mType(value::type::null) {}

    value::value(std::nullptr_t) : mType(value::type::null) {}

    value::value(enum value::type t) : mType(t)
    {
        switch (mType)
        {
        case type::array:
            new (&mStorage.array) std::vector<value>();
            break;
        case type::string:
            new (&mStorage.string) std::string();
            break;
        case type::float64:
            mStorage.number = 0;
            break;
        case type::boolean:
            mStorage.boolean = false;
            break;
        case type::map:
            new (&mStorage.map) std::map<std::string, value>();
            break;
        case type::binary:
            new (&mStorage.binary) std::vector<uint8_t>();
            break;
        case type::null:
        default:
            break;
        }
    }

    value::value(bool val) : mType(type::boolean)
    {
        mStorage.boolean = val;
    }

    value::value(double val) : mType(type::float64)
    {
        mStorage.number = val;
    }

    value::value(const std::string& val) : mType(type::string)
    {
        new (&mStorage.string) std::string(val);
    }

    value::value(std::string&& val) : mType(type::string)
    {
        new (&mStorage.string) std::string(std::move(val));
    }

    value::value(const char* val) : mType(type::string)
    {
        new (&mStorage.string) std::string(val);
    }

    value::value(const char* val, size_t length) : mType(type::string)
    {
        new (&mStorage.string) std::string(val, length);
    }

    value::value(const std::vector<value>& val) : mType(type::array)
    {
        new (&mStorage.array) std::vector<value>(val);
    }

    value::value(std::vector<value>&& val) : mType(type::array)
    {
        new (&mStorage.array) std::vector<value>(std::move(val));
    }

    value::value(const std::map<std::string, value>& map) : mType(type::map)
    {
        new (&mStorage.map) std::map<std::string, value>(map);
    }

    value::value(std::map<std::string, value>&& map) : mType(type::map)
    {
        new (&mStorage.map) std::map<std::string, value>(std::move(map));
    }

    value::value(const std::vector<uint8_t>& bin) : mType(type::binary)
    {
        new (&mStorage.binary) std::vector<uint8_t>(bin);
    }

    value::value(std::vector<uint8_t>&& bin) : mType(type::binary)
    {
        new (&mStorage.binary) std::vector<uint8_t>(std::move(bin));
    }

    value::value(const value& rhs)
    {
        mType = rhs.mType;
        switch (mType)
        {
        case type::array:
            new (&mStorage.array) std::vector<value>(rhs.mStorage.array);
            break;
        case type::string:
            new (&mStorage.string) std::string(rhs.mStorage.string);
            break;
        case type::float64:
            mStorage.number = rhs.mStorage.number;
            break;
        case type::boolean:
            mStorage.boolean = rhs.mStorage.boolean;
            break;
        case type::map:
            new (&mStorage.map) std::map<std::string, value>(rhs.mStorage.map);
            break;
        case type::binary:
            new (&mStorage.binary) std::vector<uint8_t>(rhs.mStorage.binary);
            break;
        case type::null:
        default:
            break;
        }
    }

    value::value(value&& rhs) noexcept
    {
        mType = std::move(rhs.mType);
        switch (mType)
        {
        case type::array:
            new (&mStorage.array) std::vector<signalr::value>(std::move(rhs.mStorage.array));
            break;
        case type::string:
            new (&mStorage.string) std::string(std::move(rhs.mStorage.string));
            break;
        case type::float64:
            mStorage.number = std::move(rhs.mStorage.number);
            break;
        case type::boolean:
            mStorage.boolean = std::move(rhs.mStorage.boolean);
            break;
        case type::map:
            new (&mStorage.map) std::map<std::string, signalr::value>(std::move(rhs.mStorage.map));
            break;
        case type::binary:
            new (&mStorage.binary) std::vector<uint8_t>(std::move(rhs.mStorage.binary));
            break;
        case type::null:
        default:
            break;
        }
    }

    value::~value()
    {
        destruct_internals();
    }

    void value::destruct_internals()
    {
        switch (mType)
        {
        case type::array:
            mStorage.array.~vector();
            break;
        case type::string:
            mStorage.string.~basic_string();
            break;
        case type::map:
            mStorage.map.~map();
            break;
        case type::binary:
            mStorage.binary.~vector();
            break;
        case type::null:
        case type::float64:
        case type::boolean:
        default:
            break;
        }
    }

    value& value::operator=(const value& rhs)
    {
        destruct_internals();

        mType = rhs.mType;
        switch (mType)
        {
        case type::array:
            new (&mStorage.array) std::vector<value>(rhs.mStorage.array);
            break;
        case type::string:
            new (&mStorage.string) std::string(rhs.mStorage.string);
            break;
        case type::float64:
            mStorage.number = rhs.mStorage.number;
            break;
        case type::boolean:
            mStorage.boolean = rhs.mStorage.boolean;
            break;
        case type::map:
            new (&mStorage.map) std::map<std::string, value>(rhs.mStorage.map);
            break;
        case type::binary:
            new (&mStorage.binary) std::vector<uint8_t>(rhs.mStorage.binary);
            break;
        case type::null:
        default:
            break;
        }

        return *this;
    }

    value& value::operator=(value&& rhs) noexcept
    {
        destruct_internals();

        mType = std::move(rhs.mType);
        switch (mType)
        {
        case type::array:
            new (&mStorage.array) std::vector<value>(std::move(rhs.mStorage.array));
            break;
        case type::string:
            new (&mStorage.string) std::string(std::move(rhs.mStorage.string));
            break;
        case type::float64:
            mStorage.number = std::move(rhs.mStorage.number);
            break;
        case type::boolean:
            mStorage.boolean = std::move(rhs.mStorage.boolean);
            break;
        case type::map:
            new (&mStorage.map) std::map<std::string, value>(std::move(rhs.mStorage.map));
            break;
        case type::binary:
            new (&mStorage.binary) std::vector<uint8_t>(std::move(rhs.mStorage.binary));
            break;
        case type::null:
        default:
            break;
        }

        return *this;
    }

    bool value::is_map() const
    {
        return mType == signalr::value::type::map;
    }

    bool value::is_double() const
    {
        return mType == signalr::value::type::float64;
    }

    bool value::is_string() const
    {
        return mType == signalr::value::type::string;
    }

    bool value::is_null() const
    {
        return mType == signalr::value::type::null;
    }

    bool value::is_array() const
    {
        return mType == signalr::value::type::array;
    }

    bool value::is_bool() const
    {
        return mType == signalr::value::type::boolean;
    }

    bool value::is_binary() const
    {
        return mType == signalr::value::type::binary;
    }

    double value::as_double() const
    {
        if (!is_double())
        {
            throw signalr_exception("object is a '" + value_type_to_string(mType) + "' expected it to be a 'float64'");
        }

        return mStorage.number;
    }

    bool value::as_bool() const
    {
        if (!is_bool())
        {
            throw signalr_exception("object is a '" + value_type_to_string(mType) + "' expected it to be a 'boolean'");
        }

        return mStorage.boolean;
    }

    const std::string& value::as_string() const
    {
        if (!is_string())
        {
            throw signalr_exception("object is a '" + value_type_to_string(mType) + "' expected it to be a 'string'");
        }

        return mStorage.string;
    }

    const std::vector<value>& value::as_array() const
    {
        if (!is_array())
        {
            throw signalr_exception("object is a '" + value_type_to_string(mType) + "' expected it to be a 'array'");
        }

        return mStorage.array;
    }

    const std::map<std::string, value>& value::as_map() const
    {
        if (!is_map())
        {
            throw signalr_exception("object is a '" + value_type_to_string(mType) + "' expected it to be a 'map'");
        }

        return mStorage.map;
    }

    const std::vector<uint8_t>& value::as_binary() const
    {
        if (!is_binary())
        {
            throw signalr_exception("object is a '" + value_type_to_string(mType) + "' expected it to be a 'binary'");
        }

        return mStorage.binary;
    }

    enum value::type value::type() const
    {
        return mType;
    }
}