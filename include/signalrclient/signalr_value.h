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
        bool is_map() const;

        /**
         * True if the object stored is double.
         */
        bool is_double() const;

        /**
         * True if the object stored is a string.
         */
        bool is_string() const;

        /**
         * True if the object stored is empty.
         */
        bool is_null() const;

        /**
         * True if the object stored is an array of signalr::value's.
         */
        bool is_array() const;

        /**
         * True if the object stored is a bool.
         */
        bool is_bool() const;

        /**
         * Returns the stored object as a double. This will throw if the underlying object is not a signalr::type::float64.
         */
        double as_double() const;

        /**
         * Returns the stored object as a bool. This will throw if the underlying object is not a signalr::type::boolean.
         */
        bool as_bool() const;

        /**
         * Returns the stored object as a string. This will throw if the underlying object is not a signalr::type::string.
         */
        const std::string& as_string() const;

        /**
         * Returns the stored object as an array of signalr::value's. This will throw if the underlying object is not a signalr::type::array.
         */
        std::vector<value> as_array() const;

        /**
         * Returns the stored object as a map of property name to signalr::value. This will throw if the underlying object is not a signalr::type::map.
         */
        std::map<std::string, value> as_map() const;

        /**
         * Returns the signalr::type that represents the stored object.
         */
        type type() const;
    };
}
