// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#pragma once

#include <exception>
#include <functional>
#include <chrono>

namespace signalr
{
    typedef std::function<void()> signalr_base_cb;

    struct scheduler
    {
        virtual void schedule(const signalr_base_cb& cb, std::chrono::milliseconds delay = std::chrono::milliseconds::zero()) = 0;

        virtual ~scheduler() {}
    };
}
