// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

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

        cancellation_token(const cancellation_token&) = delete;
        cancellation_token& operator=(const cancellation_token&) = delete;

        ~cancellation_token()
        {
            if (m_callback)
            {
                m_callback();
            }
        }

        void cancel()
        {
            std::function<void()> callback;
            {
                std::lock_guard<std::mutex> lock(m_lock);
                m_signaled = true;
                m_condition.notify_all();
                callback = m_callback;
                m_callback = nullptr;
            } // unlock

            if (callback)
            {
                callback();
            }
        }

        void reset()
        {
            std::lock_guard<std::mutex> lock(m_lock);
            assert(m_callback == nullptr);
            m_signaled = false;
            m_callback = nullptr;
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

        void throw_if_cancellation_requested() const
        {
            if (m_signaled)
            {
                throw canceled_exception();
            }
        }

        void register_callback(std::function<void()> callback)
        {
            bool run_callback = false;
            {
                std::lock_guard<std::mutex> lock(m_lock);
                assert(m_callback == nullptr);
                if (m_signaled)
                {
                    run_callback = m_signaled;
                }
                else
                {
                    m_callback = callback;
                }
            } // unlock

            if (run_callback)
            {
                callback();
            }
        }

    private:
        std::mutex m_lock;
        std::condition_variable m_condition;
        bool m_signaled;
        std::function<void()> m_callback;
    };
}
