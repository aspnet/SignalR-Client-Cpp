// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#include "stdafx.h"
#include "test_utils.h"
#include "../src/signalrclient/signalr_default_scheduler.h"

using namespace signalr;

TEST(scheduler, callbacks_run_on_different_thread)
{
    signalr_default_scheduler scheduler;

    auto current_thread = std::this_thread::get_id();
    std::thread::id id;
    auto mre = manual_reset_event<void>();
    scheduler.schedule([&id, &mre]()
        {
            id = std::this_thread::get_id();
            mre.set();
        });

    mre.get();
    ASSERT_NE(current_thread, id);
}

TEST(scheduler, callback_can_be_called_when_delayed_callback_called_first)
{
    signalr_default_scheduler scheduler;

    auto mre = manual_reset_event<void>();
    auto timer_mre = manual_reset_event<void>();
    bool first_callback = false;
    scheduler.schedule([&first_callback, &timer_mre]()
        {
            first_callback = true;
            timer_mre.set();
        }, std::chrono::milliseconds(1000));

    scheduler.schedule([&mre]()
        {
            mre.set();
        });

    mre.get();
    ASSERT_FALSE(first_callback);
    timer_mre.get();
}

TEST(scheduler, callback_with_delay_is_delayed)
{
    signalr_default_scheduler scheduler;
    auto delay = std::chrono::milliseconds(100);

    auto mre = manual_reset_event<void>();
    bool first_callback = false;
    auto prev_now = std::chrono::steady_clock::now();
    scheduler.schedule([&mre]()
        {
            mre.set();
        }, delay);

    mre.get();

    auto now = std::chrono::steady_clock::now();
    ASSERT_TRUE(now - prev_now >= delay);
}

// Makes sure state is thread safe and the scheduler doesn't seg fault when destructed
TEST(scheduler, scheduler_can_destruct_with_callbacks_registered)
{
    {
        signalr_default_scheduler scheduler;

        scheduler.schedule([]()
            {
            }, std::chrono::seconds(1));
    }

    manual_reset_event<void> start_mre{};
    manual_reset_event<void> continue_mre{};
    {
        signalr_default_scheduler scheduler;

        scheduler.schedule([&start_mre, &continue_mre]()
            {
                start_mre.set();
                continue_mre.get();

                // avoids referencing these objects after they've been destructed
                start_mre.set();
            });

        start_mre.get();
    }
    continue_mre.set();
    start_mre.get();
}