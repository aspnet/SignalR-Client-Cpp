// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#include "stdafx.h"
#include "test_websocket_client.h"
#include "test_utils.h"
#include "signalrclient/signalr_exception.h"

std::shared_ptr<test_websocket_client> create_test_websocket_client(
    std::function<void(const std::string & msg, std::function<void(std::exception_ptr)>)> send_function,
    std::function<void(const std::string&, std::function<void(std::exception_ptr)>)> connect_function,
    std::function<void(std::function<void(std::exception_ptr)>)> close_function)
{
    auto websocket_client = std::make_shared<test_websocket_client>();
    websocket_client->set_send_function(send_function);
    websocket_client->set_connect_function(connect_function);
    websocket_client->set_close_function(close_function);

    return websocket_client;
}

test_websocket_client::test_websocket_client()
    : m_connect_function(std::make_shared<std::function<void(const std::string&, std::function<void(std::exception_ptr)>)>>([](const std::string&, std::function<void(std::exception_ptr)> callback) { callback(nullptr); })),
    m_send_function(std::make_shared<std::function<void(const std::string& msg, std::function<void(std::exception_ptr)>)>>([](const std::string msg, std::function<void(std::exception_ptr)> callback) { callback(nullptr); })),
    m_close_function(std::make_shared<std::function<void(std::function<void(std::exception_ptr)>)>>([](std::function<void(std::exception_ptr)> callback) { callback(nullptr); })),
    m_receive_message_event(), m_receive_message(), m_stopped(true), receive_count(0)
{
    m_receive_loop_not_running.cancel();
}

test_websocket_client::~test_websocket_client()
{
    {
        std::lock_guard<std::mutex> lock(m_receive_lock);
        if (!m_stopped)
        {
            m_stopped = true;

            try
            {
                m_receive_message_event.set(false);
            }
            // we don't care if someone already set the promise
            // we're just trying to end the receive loop
            catch (std::future_error) {}
        }

        m_receive_loop_not_running.wait(1000);
    }
}

void test_websocket_client::set_config(const signalr_client_config& config)
{
    m_scheduler = config.get_scheduler();
}

void test_websocket_client::start(const std::string& url, std::function<void(std::exception_ptr)> callback)
{
    std::lock_guard<std::mutex> lock(m_receive_lock);
    m_stopped = false;
    m_receive_message_event = manual_reset_event<bool>();
    m_receive_waiting = manual_reset_event<void>();
    m_receive_loop_not_running.cancel();

    handshake_sent.reset();
    receive_loop_started.reset();

    auto local_copy = m_connect_function;
    m_scheduler->schedule([url, callback, local_copy]()
        {
            (*local_copy)(url, callback);
        });
}

void test_websocket_client::stop(std::function<void(std::exception_ptr)> callback)
{
    {
        std::lock_guard<std::mutex> lock(m_receive_lock);
        if (!m_stopped)
        {
            m_stopped = true;

            try
            {
                m_receive_message_event.set(false);
            }
            // we don't care if someone already set the promise
            // we're just trying to end the receive loop
            catch (std::future_error) {}
        }

        m_receive_loop_not_running.wait(1000);
    }

    handshake_sent.reset();
    receive_loop_started.reset();

    auto local_copy = m_close_function;
    m_scheduler->schedule([callback, local_copy]()
        {
            (*local_copy)(callback);
        });
}

void test_websocket_client::send(const std::string& payload, signalr::transfer_format, std::function<void(std::exception_ptr)> callback)
{
    handshake_sent.cancel();
    auto local_copy = m_send_function;
    m_scheduler->schedule([payload, callback, local_copy]()
        {
            (*local_copy)(payload, callback);
        });
}

void test_websocket_client::receive(std::function<void(const std::string&, std::exception_ptr)> callback)
{
    receive_count++;
    receive_loop_started.cancel();
    {
        std::lock_guard<std::mutex> lock(m_receive_lock);
        m_receive_loop_not_running.reset();
    }

    std::thread([callback, this]()
        {
            m_receive_waiting.set();
            if (m_stopped || !m_receive_message_event.get())
            {
                m_receive_loop_not_running.cancel();
                callback("", nullptr);
                return;
            }

            std::string message;
            std::exception_ptr exception = nullptr;
            {
                std::lock_guard<std::mutex> lock(m_receive_lock);
                message = m_receive_message;
                exception = m_receive_exception;
            }

            m_receive_loop_not_running.cancel();
            callback(message, exception);
        }).detach();
}

void test_websocket_client::set_connect_function(std::function<void(const std::string&, std::function<void(std::exception_ptr)>)> connect_function)
{
    m_connect_function = std::make_shared<std::function<void(const std::string&, std::function<void(std::exception_ptr)>)>>(connect_function);
}

void test_websocket_client::set_send_function(std::function<void(const std::string& msg, std::function<void(std::exception_ptr)>)> send_function)
{
    m_send_function = std::make_shared<std::function<void(const std::string & msg, std::function<void(std::exception_ptr)>)>>(send_function);
}

void test_websocket_client::set_close_function(std::function<void(std::function<void(std::exception_ptr)>)> close_function)
{
    m_close_function = std::make_shared<std::function<void(std::function<void(std::exception_ptr)>)>>(close_function);
}

void test_websocket_client::receive_message(const std::string& message)
{
    m_receive_waiting.get();
    std::lock_guard<std::mutex> lock(m_receive_lock);
    m_receive_message = message;
    m_receive_message_event.set(true);
}

void test_websocket_client::receive_message(std::exception_ptr ex)
{
    m_receive_waiting.get();
    std::lock_guard<std::mutex> lock(m_receive_lock);
    m_receive_exception = ex;
    m_receive_message_event.set(true);
}
