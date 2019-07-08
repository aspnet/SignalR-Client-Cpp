// Copyright (c) .NET Foundation. All rights reserved.
// Licensed under the Apache License, Version 2.0. See License.txt in the project root for license information.

namespace signalr
{
    class cancellation_token
    {
    public:
        bool is_canceled() const
        {
            return m_isCanceled;
        }

        void cancel()
        {
            m_isCanceled = true;
        }
    private:
        bool m_isCanceled;
    };
}