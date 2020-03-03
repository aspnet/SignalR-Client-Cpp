// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#pragma once

#include "_exports.h"

namespace signalr
{
    class log_writer
    {
    public:
        // NOTE: the caller does not enforce thread safety of this call
        SIGNALRCLIENT_API virtual void write(const std::string &entry) = 0;
    };
}
