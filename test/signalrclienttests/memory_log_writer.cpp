// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#include "stdafx.h"
#include "memory_log_writer.h"

void memory_log_writer::write(const std::string& entry)
{
    std::lock_guard<std::mutex> lock(m_entries_lock);
    m_log_entries.push_back(entry);
}

std::vector<std::string> memory_log_writer::get_log_entries()
{
    std::lock_guard<std::mutex> lock(m_entries_lock);
    return m_log_entries;
}