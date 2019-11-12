// Copyright (c) .NET Foundation. All rights reserved.
// Licensed under the Apache License, Version 2.0. See License.txt in the project root for license information.

#pragma once

namespace signalr
{

    enum message_type
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