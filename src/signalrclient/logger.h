// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#pragma once

#include <memory>
#include "signalrclient/trace_level.h"
#include "signalrclient/log_writer.h"

namespace signalr
{
    class logger
    {
    public:
        logger(const std::shared_ptr<log_writer>& log_writer, trace_level trace_level) noexcept;

        void log(trace_level level, const std::string& entry) const;

    private:
        std::shared_ptr<log_writer> m_log_writer;
        trace_level m_trace_level;

        static std::string translate_trace_level(trace_level trace_level);
    };
}
