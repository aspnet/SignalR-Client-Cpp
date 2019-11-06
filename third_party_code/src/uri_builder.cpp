/***
 * Copyright (C) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
 *
 * =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 *
 * Builder for constructing URIs.
 *
 * For the latest on this and related APIs, please see: https://github.com/Microsoft/cpprestsdk
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 ****/

// Copied and modified slightly from https://github.com/microsoft/cpprestsdk/blob/fd6aa00c18a7fb8dbe957175078f02346775045a/Release/src/uri/uri_builder.cpp

#include "../../src/signalrclient/stdafx.h"
#include "uri_builder.h"
#include <locale>
#include <sstream>

static const std::string oneSlash = "/";

namespace signalr
{
    uri_builder& uri_builder::append_path(const std::string& toAppend, bool do_encode)
    {
        if (!toAppend.empty() && toAppend != oneSlash)
        {
            auto& thisPath = m_uri.m_path;
            if (&thisPath == &toAppend)
            {
                auto appendCopy = toAppend;
                return append_path(appendCopy, do_encode);
            }

            if (thisPath.empty() || thisPath == oneSlash)
            {
                thisPath.clear();
                if (toAppend.front() != '/')
                {
                    thisPath.push_back('/');
                }
            }
            else if (thisPath.back() == '/' && toAppend.front() == '/')
            {
                thisPath.pop_back();
            }
            else if (thisPath.back() != '/' && toAppend.front() != '/')
            {
                thisPath.push_back('/');
            }
            else
            {
                // Only one slash.
            }

            if (do_encode)
            {
                thisPath.append(uri::encode_uri(toAppend, uri::components::path));
            }
            else
            {
                thisPath.append(toAppend);
            }
        }

        return *this;
    }

    uri_builder& uri_builder::append_path_raw(const std::string& toAppend, bool do_encode)
    {
        if (!toAppend.empty())
        {
            auto& thisPath = m_uri.m_path;
            if (&thisPath == &toAppend)
            {
                auto appendCopy = toAppend;
                return append_path_raw(appendCopy, do_encode);
            }

            if (thisPath != oneSlash)
            {
                thisPath.push_back('/');
            }

            if (do_encode)
            {
                thisPath.append(uri::encode_uri(toAppend, uri::components::path));
            }
            else
            {
                thisPath.append(toAppend);
            }
        }

        return *this;
    }

    uri_builder& uri_builder::append_query(const std::string& toAppend, bool do_encode)
    {
        if (!toAppend.empty())
        {
            auto& thisQuery = m_uri.m_query;
            if (&thisQuery == &toAppend)
            {
                auto appendCopy = toAppend;
                return append_query(appendCopy, do_encode);
            }

            if (thisQuery.empty())
            {
                thisQuery.clear();
            }
            else if (thisQuery.back() == '&' && toAppend.front() == '&')
            {
                thisQuery.pop_back();
            }
            else if (thisQuery.back() != '&' && toAppend.front() != '&')
            {
                thisQuery.push_back('&');
            }
            else
            {
                // Only one ampersand.
            }

            if (do_encode)
            {
                thisQuery.append(uri::encode_uri(toAppend, uri::components::query));
            }
            else
            {
                thisQuery.append(toAppend);
            }
        }

        return *this;
    }

    uri_builder& uri_builder::set_port(const std::string& port)
    {
        std::istringstream portStream(port);
        portStream.imbue(std::locale::classic());
        int port_tmp;
        portStream >> port_tmp;
        if (portStream.fail() || portStream.bad())
        {
            throw std::invalid_argument("invalid port argument, must be non empty string containing integer value");
        }
        m_uri.m_port = port_tmp;
        return *this;
    }

    uri_builder& uri_builder::append(const uri& relative_uri)
    {
        append_path(relative_uri.path());
        append_query(relative_uri.query());
        this->set_fragment(this->fragment() + relative_uri.fragment());
        return *this;
    }

    std::string uri_builder::to_string() const { return to_uri().to_string(); }

    uri uri_builder::to_uri() const { return uri(m_uri); }

    bool uri_builder::is_valid() { return uri::validate(m_uri.join()); }

    void uri_builder::append_query_encode_impl(const std::string& name, const std::string& value)
    {
        std::string encodedQuery = uri::encode_query_impl(name);
        encodedQuery.push_back('=');
        encodedQuery.append(uri::encode_query_impl(value));

        // The query key value pair was already encoded by us or the user separately.
        append_query(encodedQuery, false);
    }

    void uri_builder::append_query_no_encode_impl(const std::string& name, const std::string& value)
    {
        append_query(name + "=" + value, false);
    }

} // namespace signalr
