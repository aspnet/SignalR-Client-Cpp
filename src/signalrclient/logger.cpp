// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#include "stdafx.h"
#include "logger.h"
#include <iomanip>
#include <sstream>
#include <iostream>
#include <assert.h>
#include <chrono>

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
        log(level, entry.data(), entry.length());
    }

    void logger::log(trace_level level, const char* entry, size_t entry_count) const
    {
        if (level >= m_trace_level && m_log_writer)
        {
            try
            {
                std::chrono::milliseconds milliseconds =
                    std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
                std::chrono::seconds seconds = std::chrono::duration_cast<std::chrono::seconds>(milliseconds);
                milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(milliseconds - seconds);

                time_t t = seconds.count();
                tm time;
                // convert time to utc
                GMTIME(&time, &t);

                char timeString[sizeof("2019-11-23T13:23:02.000Z")];

                // format string to ISO8601 with additional space for 3 digits of millisecond precision
                std::strftime(timeString, sizeof(timeString), "%FT%T.000Z", &time);

                // add millisecond part
                // 5 = 3 digits of millisecond precision + 'Z' + null character ending
                snprintf(timeString + sizeof(timeString) - 5, 5, "%03dZ", (int)milliseconds.count());

                std::stringstream os;
                os << timeString;

                write_trace_level(level, os);

                os.write(entry, (std::streamsize)entry_count) << std::endl;
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

    void logger::write_trace_level(trace_level trace_level, std::ostream& stream)
    {
        switch (trace_level)
        {
        case signalr::trace_level::verbose:
            stream << " [verbose  ] ";
            break;
        case signalr::trace_level::debug:
            stream << " [debug    ] ";
            break;
        case signalr::trace_level::info:
            stream << " [info     ] ";
            break;
        case signalr::trace_level::warning:
            stream << " [warning  ] ";
            break;
        case signalr::trace_level::error:
            stream << " [error    ] ";
            break;
        case signalr::trace_level::critical:
            stream << " [critical ] ";
            break;
        case signalr::trace_level::none:
            stream << " [none     ] ";
            break;
        default:
            assert(false);
            stream << " [(unknown)] ";
            break;
        }
    }
}
