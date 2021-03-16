// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#pragma once

#include <functional>
#include "signalrclient/websocket_client.h"
#include "test_utils.h"
#include "cancellation_token.h"
#include <queue>

using namespace signalr;

class test_websocket_client : public websocket_client
{
public:
    test_websocket_client();
    ~test_websocket_client();

    void start(const std::string& url, std::function<void(std::exception_ptr)> callback);

    void stop(std::function<void(std::exception_ptr)> callback);

    void send(const std::string& payload, signalr::transfer_format transfer_format, std::function<void(std::exception_ptr)> callback);

    void receive(std::function<void(const std::string&, std::exception_ptr)> callback);

    void set_connect_function(std::function<void(const std::string&, std::function<void(std::exception_ptr)>)> connect_function);

    void set_send_function(std::function<void(const std::string& msg, std::function<void(std::exception_ptr)>)> send_function);

    void set_close_function(std::function<void(std::function<void(std::exception_ptr)>)> close_function);

    void receive_message(const std::string& message);
    void receive_message(std::exception_ptr ex);

    cancellation_token receive_loop_started;
    cancellation_token handshake_sent;
    int receive_count;

private:
    std::shared_ptr<std::function<void(const std::string&, std::function<void(std::exception_ptr)>)>> m_connect_function;

    std::shared_ptr < std::function<void(const std::string&, std::function<void(std::exception_ptr)>)>> m_send_function;

    std::shared_ptr < std::function<void(std::function<void(std::exception_ptr)>)>> m_close_function;

    std::exception_ptr m_receive_exception;
    std::string m_receive_message;

    manual_reset_event<bool> m_receive_message_event;
    std::mutex m_receive_lock;
    bool m_stopped;
    manual_reset_event<void> m_receive_waiting;
    cancellation_token m_receive_loop_not_running;
};

std::shared_ptr<test_websocket_client> create_test_websocket_client(
    std::function<void(const std::string & msg, std::function<void(std::exception_ptr)>)> send_function = [](const std::string&, std::function<void(std::exception_ptr)> callback) { callback(nullptr); },
    std::function<void(const std::string&, std::function<void(std::exception_ptr)>)> connect_function = [](const std::string&, std::function<void(std::exception_ptr)> callback) { callback(nullptr); },
    std::function<void(std::function<void(std::exception_ptr)>)> close_function = [](std::function<void(std::exception_ptr)> callback) { callback(nullptr); });