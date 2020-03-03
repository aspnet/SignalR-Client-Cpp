// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#pragma once

#include <stdexcept>
#include "signalr_exception.h"

namespace signalr
{
    class hub_exception : public signalr_exception
    {
    public:
        hub_exception(const std::string &what)
            : signalr_exception(what)
        {}
    };
}
