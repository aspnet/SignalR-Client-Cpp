// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#pragma once

#include <future>

namespace signalr
{
    class completion_event_impl : public std::enable_shared_from_this<completion_event_impl>
    {
    public:
        static std::shared_ptr<completion_event_impl> create()
        {
            return std::shared_ptr<completion_event_impl>(new completion_event_impl());
        }

        // no op when already set
        void set()
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (!m_isSet)
            {
                m_promise.set_value();
                m_isSet = true;
            }
        }

        // no op when already set
        void set(const std::exception_ptr& exception)
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (!m_isSet)
            {
                m_promise.set_exception(exception);
                m_isSet = true;
            }
        }

        // blocks and returns when set or throws when an exception is set
        void get() const
        {
            m_future.get();
        }

    private:
        completion_event_impl() : m_isSet(false)
        {
            m_future = m_promise.get_future().share();
        }

        std::promise<void> m_promise;
        std::shared_future<void> m_future;
        bool m_isSet;
        std::mutex m_mutex;
    };

    class completion_event
    {
    public:
        completion_event() : m_impl(completion_event_impl::create())
        {
        }

        completion_event(const completion_event& rhs)
        {
            m_impl = rhs.m_impl;
        }

        // no op when already set
        void set()
        {
            m_impl->set();
        }

        // no op when already set
        void set(const std::exception_ptr& exception)
        {
            m_impl->set(exception);
        }

        // blocks and returns when set or throws when an exception is set
        void get() const
        {
            m_impl->get();
        }
    private:
        std::shared_ptr<completion_event_impl> m_impl;
    };
}