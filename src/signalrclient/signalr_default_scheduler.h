// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#pragma once

#include "../include/signalrclient/scheduler.h"

namespace signalr
{
    struct signalr_default_scheduler : scheduler
    {
        signalr_default_scheduler() : m_callbacks(std::make_shared<std::vector<std::pair<signalr_base_cb, std::chrono::nanoseconds>>>()),
            m_timer_callbacks(std::make_shared<std::vector<std::pair<signalr_base_cb, std::chrono::nanoseconds>>>()), m_callback_lock(std::make_shared<std::mutex>()),
            m_timer_lock(std::make_shared<std::mutex>()), m_callback_cv(std::make_shared<std::condition_variable>()), m_timer_cv(std::make_shared<std::condition_variable>()),
            m_closed(std::make_shared<bool>())
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
        std::shared_ptr<std::vector<std::pair<signalr_base_cb, std::chrono::nanoseconds>>> m_callbacks;
        std::shared_ptr<std::vector<std::pair<signalr_base_cb, std::chrono::nanoseconds>>> m_timer_callbacks;
        std::shared_ptr<std::mutex> m_callback_lock;
        std::shared_ptr<std::mutex> m_timer_lock;
        std::shared_ptr<std::condition_variable> m_callback_cv;
        std::shared_ptr<std::condition_variable> m_timer_cv;
        std::shared_ptr<bool> m_closed;

        void close();
    };
}