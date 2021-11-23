// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#pragma once

namespace signalr
{
    enum class message_type
    {
        invocation = 1,
        stream_item,
        completion,
        stream_invocation,
        cancel_invocation,
        ping,
        close,
    };
}