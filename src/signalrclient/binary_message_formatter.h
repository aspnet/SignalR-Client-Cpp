// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#pragma once

#ifdef USE_MSGPACK

#include <string>

namespace signalr
{
    namespace binary_message_formatter
    {
        void write_length_prefix(std::string &);
    }
}

#endif