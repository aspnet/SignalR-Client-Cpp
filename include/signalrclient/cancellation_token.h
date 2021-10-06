// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#pragma once

#include <memory>
#include <functional>

namespace signalr
{
    class cancellation_token_source;

    class cancellation_token
    {
    public:
        /**
         * Register a callback to run when this token is canceled or destructed.
         * If the token is already canceled, the callback will run immediately.
         */
        void register_callback(std::function<void()> callback);

        /**
         * Check if the token has been canceled already.
         */
        bool is_canceled() const;
    private:
        friend cancellation_token get_cancellation_token(std::weak_ptr<cancellation_token_source> s);

        cancellation_token(std::weak_ptr<cancellation_token_source> parent)
            : mParent(parent)
        {
        }

        std::weak_ptr<cancellation_token_source> mParent;
    };
}
