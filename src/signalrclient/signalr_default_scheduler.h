// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#pragma once

#include "../include/signalrclient/scheduler.h"

namespace signalr
{
    struct thread
    {
    public:
        thread();
        void add(signalr_base_cb);
        bool is_free() const;
        void shutdown();
        ~thread();
    private:
#pragma warning( push )
#pragma warning( disable: 4625 5026 4626 5027 )
        struct internals
        {
            std::vector<signalr_base_cb> m_callbacks;
            std::mutex m_callback_lock;
            std::condition_variable m_callback_cv;
            bool m_closed;
            bool m_busy;
        };
#pragma warning( pop )

        std::shared_ptr<internals> m_internals;
    };

    struct signalr_default_scheduler : scheduler
    {
        signalr_default_scheduler() : m_internals(std::make_shared<internals>())
        {
            run();
        }
        signalr_default_scheduler(const signalr_default_scheduler&) = delete;
        signalr_default_scheduler& operator=(const signalr_default_scheduler&) = delete;

        void schedule(const signalr_cb& cb, std::chrono::milliseconds delay = std::chrono::milliseconds::zero());
        void schedule(const signalr_cb& cb, std::exception_ptr, std::chrono::milliseconds delay = std::chrono::milliseconds::zero());
        void schedule(const signalr_message_cb& cb, std::string, std::chrono::milliseconds delay = std::chrono::milliseconds::zero());
        void schedule(const signalr_message_cb& cb, std::exception_ptr, std::chrono::milliseconds delay = std::chrono::milliseconds::zero());
        void schedule(const signalr_base_cb& cb, std::chrono::milliseconds delay = std::chrono::milliseconds::zero());
        ~signalr_default_scheduler();

    private:
        void run();

#pragma warning( push )
#pragma warning( disable: 4625 5026 4626 5027 )
        struct internals
        {
            std::vector<std::pair<signalr_base_cb, std::chrono::time_point<std::chrono::steady_clock, std::chrono::nanoseconds>>> m_callbacks;
            std::mutex m_callback_lock;
            std::condition_variable m_callback_cv;
            bool m_closed;
        };
#pragma warning( pop )

        std::shared_ptr<internals> m_internals;

        void close();
    };
}