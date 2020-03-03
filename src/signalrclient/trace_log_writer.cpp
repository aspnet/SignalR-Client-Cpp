// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#include "stdafx.h"
#include "trace_log_writer.h"
#include <iostream>

namespace signalr
{
    void trace_log_writer::write(const std::string &entry)
    {
#ifdef _WIN32
        // OutputDebugString is thread safe
        OutputDebugStringA(entry.c_str());
#else
        // Note: there is no data race for standard output streams in C++ 11 but the results
        // might be garbled when the method is called concurrently from multiple threads
#ifdef _UTF16_STRINGS
        std::wclog << entry;
#else
        std::clog << entry;
#endif  // _UTF16_STRINGS

#endif  // _MS_WINDOWS
    }
}
