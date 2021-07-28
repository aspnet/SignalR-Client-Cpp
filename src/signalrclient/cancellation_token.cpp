// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#include "stdafx.h"
#include "cancellation_token_source.h"
#include "signalrclient/cancellation_token.h"

namespace signalr
{
    void cancellation_token::register_callback(std::function<void()> callback)
    {
        auto obj = mParent.lock();
        if (obj)
        {
            obj->register_callback(callback);
        }
        else
        {
            callback();
        }
    }

    bool cancellation_token::is_canceled() const
    {
        auto obj = mParent.lock();
        if (obj)
        {
            return obj->is_canceled();
        }
        return true;
    }
}
