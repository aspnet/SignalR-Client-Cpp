#pragma once

#include <string>
#include <vector>

namespace signalr
{
    class value
    {
    public:
        enum type
        {
            MAP,
            ARRAY,
            STRING,
            NUMBER,
            NULL,
            BOOL
        };

        value();
        value(int val);
        value(double val);
        value(const std::string& val);
        value(const std::vector<value>& val);
        value(const std::string& property_name, int val);
        value(const std::string& property_name, double val);
        value(const std::string& property_name, const std::string& val);
        value(const std::string& property_name, const std::vector<value>& val);

        bool is_map() const;
        bool is_number() const;
        bool is_string() const;
        bool is_null() const;
        bool is_array() const;
        bool has_property_name() const;

        double as_double() const;
        float as_float() const;
        int as_int() const;
        long as_long() const;
        std::string as_string() const;

        std::vector<value> get_properties() const;
        std::string get_property_name() const;
        type get_type() const;
    };
}