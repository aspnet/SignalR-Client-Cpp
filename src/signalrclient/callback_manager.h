// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#pragma once

#include <atomic>
#include <unordered_map>
#include <functional>
#include <mutex>
#include "signalrclient/signalr_value.h"

namespace signalr
{
    class callback_manager
    {
    public:
        explicit callback_manager(const char* dtor_error);
        ~callback_manager();

        callback_manager(const callback_manager&) = delete;
        callback_manager& operator=(const callback_manager&) = delete;

        std::string register_callback(const std::function<void(const char*, const signalr::value&)>& callback);
        bool invoke_callback(const std::string& callback_id, const char* error, const signalr::value& arguments, bool remove_callback);
        bool remove_callback(const std::string& callback_id);
        void clear(const char* error);

    private:
        std::atomic<int> m_id { 0 };
        std::unordered_map<std::string, std::function<void(const char*, const signalr::value&)>> m_callbacks;
        std::mutex m_map_lock;
        std::string m_dtor_clear_arguments;

        std::string get_callback_id();
    };
}
