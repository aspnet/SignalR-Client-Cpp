// Copyright (c) .NET Foundation. All rights reserved.
// Licensed under the Apache License, Version 2.0. See License.txt in the project root for license information.

#pragma once

#include <assert.h>
#include <condition_variable>
#include <mutex>

namespace signalr
{
    class canceled_exception : public std::exception
    {
    };

    class cancellation_token
    {
    public:
        static const unsigned int timeout_infinite = 0xFFFFFFFF;

        cancellation_token() noexcept
            : m_signaled(false)
        {
        }

        void cancel()
        {
            std::lock_guard<std::mutex> lock(m_lock);
            m_signaled = true;
            m_condition.notify_all();
        }

        void reset()
        {
            std::lock_guard<std::mutex> lock(m_lock);
            m_signaled = false;
        }

        bool is_canceled() const
        {
            return m_signaled;
        }

        unsigned int wait(unsigned int timeout)
        {
            std::unique_lock<std::mutex> lock(m_lock);
            if (timeout == cancellation_token::timeout_infinite)
            {
                m_condition.wait(lock, [this]() noexcept { return m_signaled; });
                return 0;
            }
            else
            {
                const std::chrono::milliseconds period(timeout);
                const auto status = m_condition.wait_for(lock, period, [this]() noexcept { return m_signaled; });
                assert(status == m_signaled);
                // Return 0 if the wait completed as a result of signaling the event. Otherwise, return timeout_infinite
                return status ? 0 : cancellation_token::timeout_infinite;
            }
        }

        unsigned int wait()
        {
            return wait(cancellation_token::timeout_infinite);
        }

        void throw_if_cancellation_requested()
        {
            if (m_signaled)
            {
                throw canceled_exception();
            }
        }
    private:
        std::mutex m_lock;
        std::condition_variable m_condition;
        bool m_signaled;
    };
}
