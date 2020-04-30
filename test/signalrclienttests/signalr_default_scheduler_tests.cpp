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

TEST(scheduler, callback_can_be_called_when_timer_callback_called_first)
{
    signalr_default_scheduler scheduler;

    auto mre = manual_reset_event<void>();
    bool first_callback = false;
    scheduler.schedule([&first_callback]()
        {
            first_callback = true;
        }, std::chrono::milliseconds(100));

    scheduler.schedule([&mre]()
        {
            mre.set();
        });

    mre.get();
    ASSERT_FALSE(first_callback);
}

//TEST(scheduler, scheduler_can_destruct_with_callbacks_registered)
//{
//    auto mre = manual_reset_event<void>();
//    {
//        signalr_default_scheduler scheduler;
//
//        scheduler.schedule([&mre]()
//            {
//                mre.set();
//            }, std::chrono::milliseconds(100));
//    }
//
//    mre.get();
//}