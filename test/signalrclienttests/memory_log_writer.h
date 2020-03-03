// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#pragma once

#include <vector>
#include <mutex>
#include "signalrclient/log_writer.h"

using namespace signalr;

class memory_log_writer : public log_writer
{
public:
    void write(const std::string &entry) override;
    std::vector<std::string> get_log_entries();

private:
    std::vector<std::string> m_log_entries;
    std::mutex m_entries_lock;
};