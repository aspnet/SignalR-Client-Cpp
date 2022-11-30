#pragma once

#include "signalr_value.h"
#include <stdexcept>

namespace signalr
{
    template <typename T>
    class is_map
    {
    public:
        static const bool value = false;
    };

    template <typename T>
    class is_map<std::map<std::string, T>>
    {
    public:
        static const bool value = true;
    };

    template <typename T>
    class is_vector
    {
    public:
        static const bool value = false;
    };

    template <typename T>
    class is_vector<std::vector<T>>
    {
    public:
        static const bool value = true;
    };

    template <typename T, std::enable_if_t<!is_map<std::enable_if_t<!is_vector<T>::value, T>>::value, int> = 0>
    T convert_value(const signalr::value& value)
    {
        static_assert(false, "type conversion not defined");
        throw std::runtime_error("failed");
    }

    template <>
    inline int convert_value<int>(const signalr::value& value)
    {
        if (value.is_double())
        {
            return (int)value.as_double();
        }
        throw std::runtime_error("not type");
    }

    template <>
    inline std::string convert_value<std::string>(const signalr::value& value)
    {
        if (value.is_string())
        {
            return value.as_string();
        }
        throw std::runtime_error("not type");
    }

    template <typename T>
    typename std::enable_if_t<is_map<T>::value, T> convert_value(const signalr::value& value)
    {
        if (value.is_map())
        {
            T map{};
            for (auto& v : value.as_map())
            {
                map.insert(std::make_pair(v.first, convert_value<typename T::mapped_type>(v.second)));
            }
            return map;
        }
        throw std::runtime_error("not type");
    }

    template <typename T>
    typename std::enable_if_t<is_vector<T>::value, T> convert_value(const signalr::value& value)
    {
        if (value.is_array())
        {
            T vec{};
            for (auto& v : value.as_array())
            {
                vec.push_back(convert_value<typename T::value_type>(v));
            }
            return vec;
        }
        throw std::runtime_error("not type");
    }
}