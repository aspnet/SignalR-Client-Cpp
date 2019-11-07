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
    enum class type
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
        value();
        value(int val);
        value(double val);
        value(const std::string& val);
        value(const std::vector<value>& val);
        value(const std::map<std::string, value>& map);

        /**
         * True if the object stored is a Key-Value pair.
         */
        bool is_map() const
        {
            return mType == signalr::type::MAP;
        }

        /**
         * True if the object stored is double.
         */
        bool is_double() const
        {
            return mType == signalr::type::DOUBLE;
        }

        /**
         * True if the object stored is a string.
         */
        bool is_string() const
        {
            return mType == signalr::type::STRING;
        }

        /**
         * True if the object stored is empty.
         */
        bool is_null() const
        {
            return mType == signalr::type::NULL;
        }

        /**
         * True if the object stored is an array of signalr::value's.
         */
        bool is_array() const
        {
            return mType == signalr::type::ARRAY;
        }

        /**
         * True if the object stored is a bool.
         */
        bool is_bool() const
        {
            return mType == signalr::type::BOOL;
        }

        /**
         * Returns the stored object as a double. This will throw if the underlying object is not a signalr::type::float64.
         */
        double as_double() const;

        /**
         * Returns the stored object as a bool. This will throw if the underlying object is not a signalr::type::boolean.
         */
        bool as_bool() const
        {
            if (!is_bool())
            {
                throw std::exception();
            }

            return false;
        }

        /**
         * Returns the stored object as a double. The value will be cast to a double if the underlying object is a signalr::type::NUMBER otherwise it will throw
         */
        double as_double() const
        {
            if (!is_double())
            {
                throw std::exception();
            }

            return 1.0;
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

            return "";
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

            return std::vector<value>();
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

            return std::map<std::string, value>();
        }

        /**
         * Returns the signalr::type that represents the stored object.
         */
        type type() const
        {
            return mType;
        }

    private:
        signalr::type mType;

        union storage
        {
            bool boolean;
            std::string str;
            std::vector<value> arr;
            double number;
            std::map<std::string, value> map;
        };

        storage mStorage;
    };
}
