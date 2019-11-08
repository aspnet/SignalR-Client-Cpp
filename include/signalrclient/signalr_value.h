// Copyright (c) .NET Foundation. All rights reserved.
// Licensed under the Apache License, Version 2.0. See License.txt in the project root for license information.

#pragma once

#include <string>
#include <vector>
#include <map>

namespace signalr
{
    /**
     * An enum defining the types a signalr::value may be.
     */
    enum class value_type
    {
        map,
        array,
        string,
        float64,
        null,
        boolean
    };

    /**
     * Represents a value to be provided to a SignalR method as a parameter, or returned as a return value.
     */
    class value
    {
    public:
        value() : mType(value_type::null) {}

        value(value_type t) : mType(t)
        {
            switch (mType)
            {
            case value_type::array:
                new (&mStorage.arr) std::vector<value>();
                break;
            case value_type::string:
                new (&mStorage.str) std::string();
                break;
            case value_type::float64:
                mStorage.number = 0;
                break;
            case value_type::boolean:
                mStorage.boolean = false;
                break;
            case value_type::map:
                new (&mStorage.map) std::map<std::string, value>();
                break;
            }
        }

        value(bool val) : mType(value_type::boolean)
        {
            mStorage.boolean = val;
        }

        value(int val) : mType(value_type::float64)
        {
            mStorage.number = val;
        }

        value(double val) : mType(value_type::float64)
        {
            mStorage.number = val;
        }

        value(const std::string& val) : mType(value_type::string)
        {
            new (&mStorage.str) std::string(val);
        }

        value(std::string&& val) : mType(value_type::string)
        {
            new (&mStorage.str) std::string(std::move(val));
        }

        value(const std::vector<value>& val) : mType(value_type::array)
        {
            new (&mStorage.arr) std::vector<value>(val);
        }

        value(std::vector<value>&& val) : mType(value_type::array)
        {
            new (&mStorage.arr) std::vector<value>(std::move(val));
        }

        value(const std::map<std::string, value>& map) : mType(value_type::map)
        {
            new (&mStorage.map) std::map<std::string, value>(map);
        }

        value(std::map<std::string, value>&& map) : mType(value_type::map)
        {
            new (&mStorage.map) std::map<std::string, value>(std::move(map));
        }

        value(const value& rhs)
        {
            mType = rhs.mType;
            switch (mType)
            {
            case value_type::array:
                new (&mStorage.arr) std::vector<value>(rhs.mStorage.arr);
                break;
            case value_type::string:
                new (&mStorage.str) std::string(rhs.mStorage.str);
                break;
            case value_type::float64:
                mStorage.number = rhs.mStorage.number;
                break;
            case value_type::boolean:
                mStorage.boolean = rhs.mStorage.boolean;
                break;
            case value_type::map:
                new (&mStorage.map) std::map<std::string, value>(rhs.mStorage.map);
                break;
            }
        }

        value(value&& rhs)
        {
            mType = std::move(rhs.mType);
            switch (mType)
            {
            case value_type::array:
                new (&mStorage.arr) std::vector<signalr::value>(std::move(rhs.mStorage.arr));
                break;
            case value_type::string:
                new (&mStorage.str) std::string(std::move(rhs.mStorage.str));
                break;
            case value_type::float64:
                mStorage.number = std::move(rhs.mStorage.number);
                break;
            case value_type::boolean:
                mStorage.boolean = std::move(rhs.mStorage.boolean);
                break;
            case value_type::map:
                new (&mStorage.map) std::map<std::string, signalr::value>(std::move(rhs.mStorage.map));
                break;
            }
        }

        ~value()
        {
            switch (mType)
            {
            case value_type::array:
                mStorage.arr.~vector();
                break;
            case value_type::string:
                mStorage.str.~basic_string();
                break;
            case value_type::map:
                mStorage.map.~map();
                break;
            }
        }

        value& operator=(const value& rhs)
        {
            mType = rhs.mType;
            switch (mType)
            {
            case value_type::array:
                new (&mStorage.arr) std::vector<value>(rhs.mStorage.arr);
                break;
            case value_type::string:
                new (&mStorage.str) std::string(rhs.mStorage.str);
                break;
            case value_type::float64:
                mStorage.number = rhs.mStorage.number;
                break;
            case value_type::boolean:
                mStorage.boolean = rhs.mStorage.boolean;
                break;
            case value_type::map:
                new (&mStorage.map) std::map<std::string, value>(rhs.mStorage.map);
                break;
            }

            return *this;
        }

        value& operator=(value&& rhs)
        {
            mType = std::move(rhs.mType);
            switch (mType)
            {
            case value_type::array:
                new (&mStorage.arr) std::vector<value>(std::move(rhs.mStorage.arr));
                break;
            case value_type::string:
                new (&mStorage.str) std::string(std::move(rhs.mStorage.str));
                break;
            case value_type::float64:
                mStorage.number = std::move(rhs.mStorage.number);
                break;
            case value_type::boolean:
                mStorage.boolean = std::move(rhs.mStorage.boolean);
                break;
            case value_type::map:
                new (&mStorage.map) std::map<std::string, value>(std::move(rhs.mStorage.map));
                break;
            }

            return *this;
        }

        /**
         * True if the object stored is a Key-Value pair.
         */
        bool is_map() const
        {
            return mType == signalr::value_type::map;
        }

        /**
         * True if the object stored is a double.
         */
        bool is_double() const
        {
            return mType == signalr::value_type::float64;
        }

        /**
         * True if the object stored is a string.
         */
        bool is_string() const
        {
            return mType == signalr::value_type::string;
        }

        /**
         * True if the object stored is empty.
         */
        bool is_null() const
        {
            return mType == signalr::value_type::null;
        }

        /**
         * True if the object stored is an array of signalr::value's.
         */
        bool is_array() const
        {
            return mType == signalr::value_type::array;
        }

        /**
         * True if the object stored is a bool.
         */
        bool is_bool() const
        {
            return mType == signalr::value_type::boolean;
        }

        /**
         * Returns the stored object as a double. This will throw if the underlying object is not a signalr::type::float64.
         */
        double as_double() const
        {
            if (!is_double())
            {
                throw std::exception();
            }

            return mStorage.number;
        }

        /**
         * Returns the stored object as a bool. This will throw if the underlying object is not a signalr::type::boolean.
         */
        bool as_bool() const
        {
            if (!is_bool())
            {
                throw std::exception();
            }

            return mStorage.boolean;
        }

        /**
         * Returns the stored object as a string. This will throw if the underlying object is not a signalr::type::string.
         */
        const std::string& as_string() const
        {
            if (!is_string())
            {
                throw std::exception();
            }

            return mStorage.str;
        }

        /**
         * Returns the stored object as an array of signalr::value's. This will throw if the underlying object is not a signalr::type::array.
         */
        std::vector<value> as_array() const
        {
            if (!is_array())
            {
                throw std::exception();
            }

            return mStorage.arr;
        }

        /**
         * Returns the stored object as a map of property name to signalr::value. This will throw if the underlying object is not a signalr::type::map.
         */
        std::map<std::string, value> as_map() const
        {
            if (!is_map())
            {
                throw std::exception();
            }

            return mStorage.map;
        }

        /**
         * Returns the signalr::type that represents the stored object.
         */
        value_type type() const
        {
            return mType;
        }

    private:
        value_type mType;

        union S
        {
            bool boolean;
            std::string str;
            std::vector<value> arr;
            double number;
            std::map<std::string, value> map;

            S() {}
            ~S() {}
        };

        S mStorage;
    };
}
