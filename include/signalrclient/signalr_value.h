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
        MAP,
        ARRAY,
        STRING,
        NUMBER,
        NULL,
        BOOL
    };

    /**
     * The Object Model class for storing values being passed by the user to the serializer or from the deserializer to the user.
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
         * True if the object stored is a number (int, float, char).
         */
        bool is_number() const;

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
         * Returns the stored object as a double. The value will be cast to a double if the underlying object is a signalr::type::NUMBER otherwise it will throw
         */
        double as_double() const;

        /**
         * Returns the stored object as a float. The value will be cast to a float if the underlying object is a signalr::type::NUMBER otherwise it will throw.
         */
        float as_float() const;

        /**
         * Returns the stored object as an int. The value will be cast to an int if the underlying object is a signalr::type::NUMBER otherwise it will throw.
         */
        int as_int() const;

        /**
         * Returns the stored object as a long. The value will be cast to a long if the underlying object is a signalr::type::NUMBER otherwise it will throw
         */
        long as_long() const;

        /**
         * Returns the stored object as a bool. This will throw if the underlying object is not a signalr::type::BOOL.
         */
        bool as_bool() const;

        /**
         * Returns the stored object as a string. This will throw if the underlying object is not a signalr::type::STRING.
         */
        const std::string& as_string() const;

        /**
         * Returns the stored object as an array of signalr::value's. This will throw if the underlying object is not a signalr::type::ARRAY.
         */
        std::vector<value> as_array() const;

        /**
         * Returns the stored object as a map of property name to signalr::value. This will throw if the underlying object is not a signalr::type::MAP.
         */
        std::map<std::string, value> as_map() const;

        /**
         * Returns the signalr::type that represents the stored object.
         */
        type type() const;
    };
}