// Copyright (c) .NET Foundation. All rights reserved.
// Licensed under the Apache License, Version 2.0. See License.txt in the project root for license information.

#pragma once

#include <future>

namespace signalr
{
    class completion_event
    {
    public:
        completion_event() : m_isSet(false), m_promise(std::make_shared<std::promise<void>>()), m_mutex(std::make_shared<std::mutex>())
        {
            m_future = m_promise->get_future().share();
        }

        completion_event(const completion_event& rhs)
        {
            m_future = rhs.m_future;
            m_promise = rhs.m_promise;
            m_isSet = rhs.m_isSet;
            m_mutex = rhs.m_mutex;
        }

        // no op when already set
        void set()
        {
            std::lock_guard<std::mutex> lock(*m_mutex);
            if (!m_isSet)
            {
                m_promise->set_value();
                m_isSet = true;
            }
        }

        // no op when already set
        void set(const std::exception_ptr& exception)
        {
            std::lock_guard<std::mutex> lock(*m_mutex);
            if (!m_isSet)
            {
                m_promise->set_exception(exception);
                m_isSet = true;
            }
        }

        // blocks and returns when set or throws when an exception is set
        void get() const
        {
            m_future.get();
        }
    private:
        std::shared_ptr<std::promise<void>> m_promise;
        std::shared_future<void> m_future;
        bool m_isSet;
        std::shared_ptr<std::mutex> m_mutex;
    };
}