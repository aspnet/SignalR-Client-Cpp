// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#pragma once

#include <stdexcept>

namespace signalr
{
    class signalr_exception : public std::runtime_error
    {
    public:
        explicit signalr_exception(const std::string &what)
            : runtime_error(what)
        {}
    };
}
