// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#pragma once

namespace signalr
{
    enum class trace_level : int
    {
        none = 0,
        messages = 1,
        events = 2,
        state_changes = 4,
        errors = 8,
        info = 16,
        all = messages | events | state_changes | errors | info
    };

    inline trace_level operator|(trace_level lhs, trace_level rhs) noexcept
    {
        return static_cast<trace_level>(static_cast<int>(lhs) | static_cast<int>(rhs));
    }

    inline trace_level operator&(trace_level lhs, trace_level rhs) noexcept
    {
        return static_cast<trace_level>(static_cast<int>(lhs) & static_cast<int>(rhs));
    }
}
