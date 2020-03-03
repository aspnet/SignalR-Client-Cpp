// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#pragma once

#include <stdexcept>

namespace signalr
{
    class web_exception : public std::runtime_error
    {
    public:
        web_exception(const std::string &what, unsigned short status_code)
            : runtime_error(what), m_status_code(status_code)
        {}

        unsigned short status_code() const noexcept
        {
            return m_status_code;
        }

    private:
        unsigned short m_status_code;
    };
}
