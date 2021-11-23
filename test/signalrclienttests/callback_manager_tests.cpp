// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#include "stdafx.h"
#include "callback_manager.h"

using namespace signalr;

TEST(callback_manager_register_callback, register_returns_unique_callback_ids)
{
    callback_manager callback_mgr{ "" };
    auto callback_id1 = callback_mgr.register_callback([](const char*, const signalr::value&){});
    auto callback_id2 = callback_mgr.register_callback([](const char*, const signalr::value&){});

    ASSERT_NE(callback_id1, callback_id2);
}

TEST(callback_manager_invoke_callback, invoke_callback_invokes_and_removes_callback_if_remove_callback_true)
{
    callback_manager callback_mgr{ "" };

    int callback_argument;

    auto callback_id = callback_mgr.register_callback(
        [&callback_argument](const char* error, const signalr::value& argument)
        {
            ASSERT_EQ(nullptr, error);
            callback_argument = (int)argument.as_double();
        });

    auto callback_found = callback_mgr.invoke_callback(callback_id, nullptr, signalr::value(42.0), true);

    ASSERT_TRUE(callback_found);
    ASSERT_EQ(42, callback_argument);
    ASSERT_FALSE(callback_mgr.remove_callback(callback_id));
}

TEST(callback_manager_invoke_callback, invoke_callback_invokes_and_does_not_remove_callback_if_remove_callback_false)
{
    callback_manager callback_mgr{ "" };

    int callback_argument;

    auto callback_id = callback_mgr.register_callback(
        [&callback_argument](const char* error, const signalr::value& argument)
        {
            ASSERT_EQ(nullptr, error);
            callback_argument = (int)argument.as_double();
        });

    auto callback_found = callback_mgr.invoke_callback(callback_id, nullptr, signalr::value(42.0), false);

    ASSERT_TRUE(callback_found);
    ASSERT_EQ(42, callback_argument);
    ASSERT_TRUE(callback_mgr.remove_callback(callback_id));
}

TEST(callback_manager_invoke_callback, invoke_callback_returns_false_for_invalid_callback_id)
{
    callback_manager callback_mgr{ "" };
    auto callback_found = callback_mgr.invoke_callback("42", nullptr, signalr::value(), true);

    ASSERT_FALSE(callback_found);
}

TEST(callback_manager_remove, remove_removes_callback_and_returns_true_for_valid_callback_id)
{
    auto callback_called = false;

    {
        callback_manager callback_mgr{ "" };

        auto callback_id = callback_mgr.register_callback(
            [&callback_called](const char*, const signalr::value&)
        {
            callback_called = true;
        });

        ASSERT_TRUE(callback_mgr.remove_callback(callback_id));
    }

    ASSERT_FALSE(callback_called);
}

TEST(callback_manager_remove, remove_returns_false_for_invalid_callback_id)
{
    callback_manager callback_mgr{ "" };
    ASSERT_FALSE(callback_mgr.remove_callback("42"));
}

TEST(callback_manager_clear, clear_invokes_all_callbacks)
{
    callback_manager callback_mgr{ "" };
    auto invocation_count = 0;

    for (auto i = 0; i < 10; i++)
    {
        callback_mgr.register_callback(
            [&invocation_count](const char* error, const signalr::value& argument)
            {
                invocation_count++;
                ASSERT_STREQ("clearing callback", error);
                ASSERT_TRUE(argument.is_null());
            });
    }

    callback_mgr.clear("clearing callback");

    ASSERT_EQ(10, invocation_count);
}

TEST(callback_manager_dtor, clear_invokes_all_callbacks)
{
    auto invocation_count = 0;

    {
        callback_manager callback_mgr{ "error" };
        for (auto i = 0; i < 10; i++)
        {
            callback_mgr.register_callback(
                [&invocation_count](const char* error, const signalr::value& argument)
            {
                invocation_count++;
                ASSERT_STREQ("error", error);
                ASSERT_TRUE(argument.is_null());
            });
        }
    }

    ASSERT_EQ(10, invocation_count);
}
