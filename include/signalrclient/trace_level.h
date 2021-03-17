// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#pragma once

namespace signalr
{
    enum class trace_level : int
    {
        verbose = 0,
        debug = 1,
        info = 2,
        warning = 3,
        error = 4,
        critical = 5,
        none = 6,
    };
}
