// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#pragma once

#include <assert.h>
#include <condition_variable>
#include <mutex>
#include <vector>

namespace signalr
{
    class canceled_exception : public std::exception
    {
    };

    class cancellation_token_source
    {
    public:
        static const unsigned int timeout_infinite = 0xFFFFFFFF;

        cancellation_token_source() noexcept
            : m_signaled(false)
        {
        }

        cancellation_token_source(const cancellation_token_source&) = delete;
        cancellation_token_source& operator=(const cancellation_token_source&) = delete;

        ~cancellation_token_source()
        {
            if (!m_callbacks.empty())
            {
                for (auto& func : m_callbacks)
                {
                    try
                    {
                        func();
                    }
                    catch (...)
                    {
                        assert(false);
                    }
                }
            }
        }

        void cancel()
        {
            std::vector<std::function<void()>> callbacks;
            {
                std::lock_guard<std::mutex> lock(m_lock);
                m_signaled = true;
                m_condition.notify_all();
                callbacks = std::move(m_callbacks);
                m_callbacks = std::vector<std::function<void()>>();
            } // unlock

            if (!callbacks.empty())
            {
                for (auto& func : callbacks)
                {
                    try
                    {
                        func();
                    }
                    catch (...)
                    {
                        assert(false);
                    }
                }
            }
        }

        void reset()
        {
            std::lock_guard<std::mutex> lock(m_lock);
            assert(m_callbacks.empty());
            m_signaled = false;
            m_callbacks.clear();
        }

        bool is_canceled() const
        {
            return m_signaled;
        }

        unsigned int wait(unsigned int timeout)
        {
            std::unique_lock<std::mutex> lock(m_lock);
            if (timeout == cancellation_token_source::timeout_infinite)
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
                return status ? 0 : cancellation_token_source::timeout_infinite;
            }
        }

        unsigned int wait()
        {
            return wait(cancellation_token_source::timeout_infinite);
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
                if (m_signaled)
                {
                    run_callback = m_signaled;
                }
                else
                {
                    m_callbacks.push_back(callback);
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
        std::vector<std::function<void()>> m_callbacks;
    };
}
