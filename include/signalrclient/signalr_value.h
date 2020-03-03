// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

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
        /**
         * Create an object representing a value_type::null value.
         */
        value();

        /**
         * Create an object representing a default value for the given value_type.
         */
        value(value_type t);

        /**
         * Create an object representing a value_type::boolean with the given bool value.
         */
        value(bool val);

        /**
         * Create an object representing a value_type::float64 with the given double value.
         */
        value(double val);

        /**
         * Create an object representing a value_type::string with the given string value.
         */
        value(const std::string& val);

        /**
         * Create an object representing a value_type::string with the given string value.
         */
        value(std::string&& val);

        /**
         * Create an object representing a value_type::string with the given string value.
         */
        value(const char* val);

        /**
         * Create an object representing a value_type::array with the given vector of value's.
         */
        value(const std::vector<value>& val);

        /**
         * Create an object representing a value_type::array with the given vector of value's.
         */
        value(std::vector<value>&& val);

        /**
         * Create an object representing a value_type::map with the given map of string-value's.
         */
        value(const std::map<std::string, value>& map);

        /**
         * Create an object representing a value_type::map with the given map of string-value's.
         */
        value(std::map<std::string, value>&& map);

        /**
         * Copies an existing value.
         */
        value(const value& rhs);

        /**
         * Moves an existing value.
         */
        value(value&& rhs) noexcept;

        /**
         * Cleans up the resources associated with the value.
         */
        ~value();

        /**
         * Copies an existing value.
         */
        value& operator=(const value& rhs);

        /**
         * Moves an existing value.
         */
        value& operator=(value&& rhs) noexcept;

        /**
         * True if the object stored is a Key-Value pair.
         */
        bool is_map() const;

        /**
         * True if the object stored is a double.
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
        const std::vector<value>& as_array() const;

        /**
         * Returns the stored object as a map of property name to signalr::value. This will throw if the underlying object is not a signalr::type::map.
         */
        const std::map<std::string, value>& as_map() const;

        /**
         * Returns the signalr::type that represents the stored object.
         */
        value_type type() const;

    private:
        value_type mType;

        union storage
        {
            bool boolean;
            std::string string;
            std::vector<value> array;
            double number;
            std::map<std::string, value> map;

            storage() {}
            ~storage() {}
        };

        storage mStorage;

        void destruct_internals();
    };
}
