// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#include "stdafx.h"
#include "test_utils.h"
#include "trace_log_writer.h"
#include "logger.h"
#include "memory_log_writer.h"
#include <regex>

using namespace signalr;
TEST(logger_write, entry_added_if_trace_level_set)
{
    std::shared_ptr<log_writer> writer(std::make_shared<memory_log_writer>());

    logger l(writer, trace_level::info);
    l.log(trace_level::info, "message");

    auto log_entries = std::dynamic_pointer_cast<memory_log_writer>(writer)->get_log_entries();

    ASSERT_EQ(1U, log_entries.size());
}

TEST(logger_write, entry_not_added_if_trace_level_not_high_enough)
{
    std::shared_ptr<log_writer> writer(std::make_shared<memory_log_writer>());

    logger l(writer, trace_level::info);
    l.log(trace_level::debug, "event");

    auto log_entries = std::dynamic_pointer_cast<memory_log_writer>(writer)->get_log_entries();

    ASSERT_TRUE(log_entries.empty());
}

TEST(logger_write, entries_formatted_correctly)
{
    std::shared_ptr<log_writer> writer(std::make_shared<memory_log_writer>());

    logger l(writer, trace_level::verbose);
    l.log(trace_level::info, "message");

    auto log_entries = std::dynamic_pointer_cast<memory_log_writer>(writer)->get_log_entries();
    ASSERT_FALSE(log_entries.empty());

    auto entry = log_entries[0];

    //0000-0-00T00:00:00.000Z [info     ] message\n
    std::regex r{ "[\\d]{4}-[\\d]{2}-[\\d]{2}T[\\d]{2}:[\\d]{2}:[\\d]{2}.[\\d]{3}Z \\[info     \\] message\\n" };
    ASSERT_TRUE(std::regex_match(entry, r));

    auto expected_message = std::string("[info     ] message\n");
    auto pos = entry.find(expected_message);
    ASSERT_EQ(expected_message.size(), entry.size() - pos);
}
