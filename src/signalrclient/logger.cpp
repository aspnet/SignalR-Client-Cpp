// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#include "stdafx.h"
#include "logger.h"
#include <iomanip>
#include <sstream>
#include <iostream>
#include <assert.h>

namespace signalr
{
#ifdef WIN32
#define GMTIME(r_tm, r_time_t) gmtime_s(r_tm, r_time_t)
#else
#define GMTIME(r_tm, r_time_t) gmtime_r(r_time_t, r_tm)
#endif

    logger::logger(const std::shared_ptr<log_writer>& log_writer, trace_level trace_level) noexcept
        : m_log_writer(log_writer), m_trace_level(trace_level)
    { }

    void logger::log(trace_level level, const std::string& entry) const
    {
        if ((level & m_trace_level) != trace_level::none && m_log_writer)
        {
            try
            {
                time_t t;
                tm time;
                // gets current calendar time
                std::time(&t);
                // convert time to utc
                GMTIME(&time, &t);

                // TODO: millisecond "precision"
                char timeString[sizeof("2019-11-23T13:23:02Z")];

                // format string to ISO8601
                std::strftime(timeString, sizeof(timeString), "%FT%TZ", &time);

                std::stringstream os;
                os << timeString << " ["
                    << std::left << std::setw(12) << translate_trace_level(level) << "] "<< entry << std::endl;
                m_log_writer->write(os.str());
            }
            catch (const std::exception &e)
            {
                std::cerr << "error occurred when logging: " << e.what()
                    << std::endl << "    entry: " << entry << std::endl;
            }
            catch (...)
            {
                std::cerr << "unknown error occurred when logging" << std::endl << "    entry: " << entry << std::endl;
            }
        }
    }

    std::string logger::translate_trace_level(trace_level trace_level)
    {
        switch (trace_level)
        {
        case signalr::trace_level::messages:
            return "message";
        case signalr::trace_level::state_changes:
            return "state change";
        case signalr::trace_level::events:
            return "event";
        case signalr::trace_level::errors:
            return "error";
        case signalr::trace_level::info:
            return "info";
        case signalr::trace_level::none:
            return "none";
        case signalr::trace_level::all:
            return "all";
        default:
            assert(false);
            return "(unknown)";
        }
    }
}
