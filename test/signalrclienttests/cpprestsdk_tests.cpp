// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#include "stdafx.h"
#include "connection_impl.h"
#include "test_utils.h"
#include "memory_log_writer.h"

using namespace signalr;

TEST(cpprestsdk, invalid_url_errors)
{
    auto connection = connection_impl::create("/path", trace_level::debug, std::make_shared<memory_log_writer>(), nullptr, nullptr, false);

    manual_reset_event<void> mre;
    connection->start([&mre](std::exception_ptr ex)
        {
            mre.set(ex);
        });

    try
    {
        mre.get();
        ASSERT_TRUE(false);
    }
    catch (const std::exception& ex)
    {
        ASSERT_STREQ("URI must contain a hostname.", ex.what());
    }
}