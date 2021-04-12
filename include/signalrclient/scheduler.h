// Copyright (c) .NET Foundation. All rights reserved.
// Licensed under the Apache License, Version 2.0. See License.txt in the project root for license information.

#pragma once

#include <condition_variable>
#include <exception>
#include <functional>
#include <mutex>
#include <vector>
#include <chrono>

namespace signalr
{
    typedef std::function<void()> signalr_base_cb;
    typedef std::function<void(std::exception_ptr)> signalr_cb;
    typedef std::function<void(std::string, std::exception_ptr)> signalr_message_cb;

    struct scheduler
    {
        virtual void schedule(const signalr_cb& cb, std::chrono::milliseconds delay = std::chrono::milliseconds::zero())
        {
            schedule(cb, nullptr, delay);
        }

        virtual void schedule(const signalr_cb& cb, std::exception_ptr exception, std::chrono::milliseconds delay = std::chrono::milliseconds::zero())
        {
            schedule([cb, exception]() { cb(exception); }, delay);
        }

        virtual void schedule(const signalr_message_cb& cb, std::string message, std::chrono::milliseconds delay = std::chrono::milliseconds::zero())
        {
            schedule([cb, message]() { cb(message, nullptr); }, delay);
        }

        virtual void schedule(const signalr_message_cb& cb, std::exception_ptr exception, std::chrono::milliseconds delay = std::chrono::milliseconds::zero())
        {
            schedule([cb, exception]() { cb("", exception); }, delay);
        }

        virtual void schedule(const signalr_base_cb& cb, std::chrono::milliseconds delay = std::chrono::milliseconds::zero()) = 0;

        virtual ~scheduler() {}
    };
}
