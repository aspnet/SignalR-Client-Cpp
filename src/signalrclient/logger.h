// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#pragma once

#include <memory>
#include "signalrclient/log_level.h"
#include "signalrclient/log_writer.h"

namespace signalr
{
    class logger
    {
    public:
        logger(const std::shared_ptr<log_writer>& log_writer, log_level log_level) noexcept;

        void log(log_level level, const char* entry, size_t entry_count) const;

        // TODO: variadic 'const char*' overload to avoid std::string allocations
        void log(log_level level, const std::string& entry) const;

        template <size_t N>
        void log(log_level level, const char (&entry)[N]) const
        {
            // remove '\0'
            log(level, entry, N - 1);
        }

        bool is_enabled(log_level level) const
        {
            return level >= m_log_level;
        }

    private:
        std::shared_ptr<log_writer> m_log_writer;
        log_level m_log_level;

        static void write_log_level(log_level log_level, std::ostream& stream);
    };
}
