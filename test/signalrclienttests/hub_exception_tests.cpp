// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#include "stdafx.h"
#include "signalrclient/hub_exception.h"

using namespace signalr;

TEST(hub_exception_initialization, hub_exception_initialized_correctly)
{
    auto e = hub_exception{ "error" };

    ASSERT_STREQ("error", e.what());
}
