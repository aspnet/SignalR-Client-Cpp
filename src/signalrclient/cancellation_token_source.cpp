// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#include "stdafx.h"
#include "cancellation_token_source.h"

namespace signalr
{
    const unsigned int cancellation_token_source::timeout_infinite = 0xFFFFFFFF;

    cancellation_token get_cancellation_token(std::weak_ptr<cancellation_token_source> s)
    {
        return cancellation_token(s);
    }
}