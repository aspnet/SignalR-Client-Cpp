// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#include "stdafx.h"
#include <type_traits>
#include "cancellation_token_source.h"
#include "test_utils.h"

using namespace signalr;

TEST(cancellation_token_source, defaults_to_not_canceled)
{
    cancellation_token_source cts;
    ASSERT_FALSE(cts.is_canceled());
}

TEST(cancellation_token_source, is_not_copyable)
{
    ASSERT_FALSE(std::is_copy_assignable<cancellation_token_source>::value);
    ASSERT_FALSE(std::is_copy_constructible<cancellation_token_source>::value);
}

// This would be fine to support in the future if ever needed
TEST(cancellation_token_source, is_not_moveable)
{
    ASSERT_FALSE(std::is_move_assignable<cancellation_token_source>::value);
    ASSERT_FALSE(std::is_move_constructible<cancellation_token_source>::value);
}

TEST(cancellation_token_source, cancel_sets_canceled)
{
    cancellation_token_source cts;
    cts.cancel();
    ASSERT_TRUE(cts.is_canceled());
}

TEST(cancellation_token_source, can_be_reset)
{
    cancellation_token_source cts;
    cts.cancel();
    cts.reset();
    ASSERT_FALSE(cts.is_canceled());
}

TEST(cancellation_token_source, callback_called_on_cancel)
{
    cancellation_token_source cts;
    bool called = false;
    cts.register_callback([&called]()
        {
            called = true;
        });
    cts.cancel();
    ASSERT_TRUE(called);
    ASSERT_TRUE(cts.is_canceled());
}

TEST(cancellation_token_source, callback_not_called_on_destruct)
{
    bool called = false;
    {
        cancellation_token_source cts;
        cts.register_callback([&called]()
            {
                called = true;
            });
    }
    ASSERT_FALSE(called);
}

TEST(cancellation_token_source, callback_called_if_already_canceled)
{
    cancellation_token_source cts;
    cts.cancel();
    ASSERT_TRUE(cts.is_canceled());

    bool called = false;
    cts.register_callback([&called]()
        {
            called = true;
        });
    ASSERT_TRUE(called);
}

TEST(cancellation_token_source, can_register_multiple_callbacks)
{
    cancellation_token_source cts;
    bool called = false;
    cts.register_callback([&called]()
        {
            called = true;
        });
    bool called2 = false;
    cts.register_callback([&called2]()
        {
            called2 = true;
        });

    ASSERT_FALSE(called);
    ASSERT_FALSE(called2);

    cts.cancel();
    ASSERT_TRUE(cts.is_canceled());
    ASSERT_TRUE(called);
    ASSERT_TRUE(called2);
}

TEST(cancellation_token_source, can_wait_for_cancel)
{
    cancellation_token_source cts;

    auto thread = std::thread([&cts]()
        {
            cts.cancel();
        });
    thread.join();

    ASSERT_EQ(0, cts.wait());
    ASSERT_TRUE(cts.is_canceled());
}

TEST(cancellation_token_source, can_timeout_wait)
{
    cancellation_token_source cts;

    auto mre = manual_reset_event<void>();
    auto thread = std::thread([&cts, &mre]()
        {
            mre.get();
            cts.cancel();
        });

    ASSERT_EQ(cancellation_token_source::timeout_infinite, cts.wait(100));
    ASSERT_FALSE(cts.is_canceled());

    mre.set();
    thread.join();

    ASSERT_TRUE(cts.is_canceled());
}

TEST(cancellation_token_source, throw_method_throws_if_canceled)
{
    cancellation_token_source cts;

    // noop
    cts.throw_if_cancellation_requested();

    cts.cancel();
    try
    {
        cts.throw_if_cancellation_requested();
        ASSERT_TRUE(false);
    }
    catch (canceled_exception)
    {
    }
}

TEST(cancellation_token_source, throws_if_callbacks_throw)
{
    cancellation_token_source cts;

    cts.register_callback([]()
        {
            throw std::runtime_error("error from callback");
        });

    cts.register_callback([]()
        {
            throw std::runtime_error("error from second callback");
        });

    try
    {
        cts.cancel();
        ASSERT_TRUE(false);
    }
    catch (const aggregate_exception& ex)
    {
        ASSERT_STREQ("error from callback\nerror from second callback\n", ex.what());
    }
}

TEST(cancellation_token, can_be_canceled)
{
    auto cts = std::make_shared<cancellation_token_source>();
    auto token = get_cancellation_token(cts);

    ASSERT_FALSE(token.is_canceled());

    cts->cancel();

    ASSERT_TRUE(token.is_canceled());
}

TEST(cancellation_token, can_register_callbacks)
{
    auto cts = std::make_shared<cancellation_token_source>();
    auto token = get_cancellation_token(cts);
    bool called = false;
    token.register_callback([&called]()
        {
            called = true;
        });
    bool called2 = false;
    token.register_callback([&called2]()
        {
            called2 = true;
        });

    ASSERT_FALSE(called);
    ASSERT_FALSE(called2);

    cts->cancel();

    ASSERT_TRUE(token.is_canceled());
    ASSERT_TRUE(called);
    ASSERT_TRUE(called2);
}

TEST(cancellation_token, is_canceled_with_destructed_cts)
{
    auto cts = std::make_shared<cancellation_token_source>();
    auto token = get_cancellation_token(cts);

    cts.reset();

    ASSERT_TRUE(cts == nullptr);
    ASSERT_TRUE(token.is_canceled());
}

TEST(cancellation_token, is_copyable_and_moveable)
{
    ASSERT_TRUE(std::is_copy_assignable<cancellation_token>::value);
    ASSERT_TRUE(std::is_copy_constructible<cancellation_token>::value);

    ASSERT_TRUE(std::is_move_assignable<cancellation_token>::value);
    ASSERT_TRUE(std::is_move_constructible<cancellation_token>::value);
}

TEST(cancellation_token, callback_called_when_already_canceled)
{
    auto cts = std::make_shared<cancellation_token_source>();
    auto token = get_cancellation_token(cts);

    cts->cancel();
    ASSERT_TRUE(token.is_canceled());

    bool called = false;
    token.register_callback([&called]()
        {
            called = true;
        });
    ASSERT_TRUE(called);
}

TEST(cancellation_token, callback_called_when_cts_already_destructed)
{
    auto cts = std::make_shared<cancellation_token_source>();
    auto token = get_cancellation_token(cts);

    cts.reset();

    bool called = false;
    token.register_callback([&called]()
        {
            called = true;
        });
    ASSERT_TRUE(called);
}