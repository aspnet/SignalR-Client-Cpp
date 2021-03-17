// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#pragma once

#include "signalrclient/websocket_client.h"
#include "signalrclient/http_client.h"
#include <future>

std::string remove_date_from_log_entry(const std::string &log_entry);
bool has_log_entry(const std::string& log_entry, const std::vector<std::string>& logs);

std::unique_ptr<signalr::http_client> create_test_http_client();
std::string create_uri();
std::string create_uri(const std::string& query_string);
std::vector<std::string> filter_vector(const std::vector<std::string>& source, const std::string& string);
std::string dump_vector(const std::vector<std::string>& source);

template <typename T>
class manual_reset_event
{
public:
    void set(T value)
    {
        m_promise.set_value(value);
    }

    void set(const std::exception& exception)
    {
        m_promise.set_exception(std::make_exception_ptr(exception));
    }

    void set(std::exception_ptr exception)
    {
        m_promise.set_exception(exception);
    }

    T get()
    {
        // TODO: timeout
        try
        {
            auto ret = m_promise.get_future().get();
            m_promise = std::promise<T>();
            return ret;
        }
        catch (...)
        {
            m_promise = std::promise<T>();
            std::rethrow_exception(std::current_exception());
        }
    }
private:
    std::promise<T> m_promise;
};

template <>
class manual_reset_event<void>
{
public:
    void set()
    {
        m_promise.set_value();
    }

    void set(const std::exception& exception)
    {
        m_promise.set_exception(std::make_exception_ptr(exception));
    }

    void set(std::exception_ptr exception)
    {
        if (exception != nullptr)
        {
            m_promise.set_exception(exception);
        }
        else
        {
            m_promise.set_value();
        }
    }

    void get()
    {
        try
        {
            m_promise.get_future().get();
        }
        catch (...)
        {
            m_promise = std::promise<void>();
            std::rethrow_exception(std::current_exception());
        }

        m_promise = std::promise<void>();
    }
private:
    std::promise<void> m_promise;
};
