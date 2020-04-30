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
        virtual void schedule(const signalr_cb& cb, std::chrono::milliseconds delay) = 0;
        virtual void schedule(const signalr_cb& cb, std::exception_ptr, std::chrono::milliseconds delay) = 0;
        virtual void schedule(const signalr_message_cb& cb, std::string, std::chrono::milliseconds delay) = 0;
        virtual void schedule(const signalr_message_cb& cb, std::exception_ptr, std::chrono::milliseconds delay) = 0;
        virtual void schedule(const signalr_base_cb& cb, std::chrono::milliseconds delay) = 0;

        virtual ~scheduler() {}
    };
}
