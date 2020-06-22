// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#include "stdafx.h"
#include "signalr_default_scheduler.h"
#include <thread>

namespace signalr
{
    thread::thread()
        : m_callbacks(std::make_shared<std::vector<std::pair<signalr_base_cb, std::chrono::time_point<std::chrono::steady_clock, std::chrono::nanoseconds>>>>()),
        m_callback_lock(std::make_shared<std::mutex>()), m_callback_cv(std::make_shared<std::condition_variable>()), m_closed(std::make_shared<bool>()), m_busy(std::make_shared<bool>())
    {
        auto closed = m_closed;
        auto callbacks = m_callbacks;
        auto callback_lock = m_callback_lock;
        auto cv = m_callback_cv;
        auto busy = m_busy;

        std::thread([=]()
            {
                while ((*closed) == false)
                {
                    signalr_base_cb cb;
                    {
                        std::unique_lock<std::mutex> lock((*callback_lock));
                        cv->wait(lock, [&callbacks, &closed] { return *closed || !callbacks->empty(); });
                        if ((*closed))
                        {
                            return;
                        }
                        cb = callbacks->front().first;
                        callbacks->erase(callbacks->begin());
                    } // unlock

                    try
                    {
                        cb();
                    }
                    catch (...)
                    {
                        // ignore exceptions?
                    }
                    (*busy) = false;
                }
            }).detach();
    }

    void thread::add(std::pair<signalr_base_cb, std::chrono::time_point<std::chrono::steady_clock, std::chrono::nanoseconds>> cb)
    {
        {
            std::lock_guard<std::mutex> lock((*m_callback_lock));
            m_callbacks->push_back(cb);
            (*m_busy) = true;
        } // unlock
        m_callback_cv->notify_one();
    }

    void thread::shutdown()
    {
        (*m_closed) = true;
        m_callback_cv->notify_one();
    }

    bool thread::is_free() const
    {
        return !(*m_busy);
    }

    thread::~thread()
    {
        shutdown();
    }

    void signalr_default_scheduler::schedule(const signalr_cb& cb, std::chrono::milliseconds delay)
    {
        schedule([cb]() { cb(nullptr); }, delay);
    }

    void signalr_default_scheduler::schedule(const signalr_cb& cb, std::exception_ptr exception, std::chrono::milliseconds delay)
    {
        schedule([cb, exception]() { cb(exception); }, delay);
    }

    void signalr_default_scheduler::schedule(const signalr_message_cb& cb, std::string message, std::chrono::milliseconds delay)
    {
        schedule([cb, message]() { cb(message, nullptr); }, delay);
    }

    void signalr_default_scheduler::schedule(const signalr_message_cb& cb, std::exception_ptr exception, std::chrono::milliseconds delay)
    {
        schedule([cb, exception]() { cb("", exception); }, delay);
    }

    void signalr_default_scheduler::schedule(const signalr_base_cb& cb, std::chrono::milliseconds delay)
    {
        {
            std::lock_guard<std::mutex> lock((*m_callback_lock));
            m_callbacks->push_back(std::make_pair(cb, std::chrono::steady_clock::now() + delay));
        } // unlock
        m_callback_cv->notify_one();
    }

    void signalr_default_scheduler::run()
    {
        auto closed = m_closed;
        auto callbacks = m_callbacks;
        auto callback_lock = m_callback_lock;
        auto cv = m_callback_cv;

        std::thread([callbacks, callback_lock, cv, closed]()
            {
                std::vector<thread> threads{ 5 };

                while ((*closed) == false)
                {
                    std::pair<signalr::signalr_base_cb, std::chrono::time_point<std::chrono::steady_clock, std::chrono::nanoseconds>> cb;
                    {
                        std::unique_lock<std::mutex> lock((*callback_lock));
                        cv->wait_for(lock, std::chrono::milliseconds(15), [&callbacks, &closed] { return (*closed) || !callbacks->empty(); });
                        if ((*closed))
                        {
                            return;
                        }

                        // find the first callback that is ready to run and remove it from the list
                        // then exit the lock so other threads can find a callback
                        auto curr_time = std::chrono::steady_clock::now();
                        auto it = callbacks->begin();
                        auto found = false;
                        while (it != callbacks->end())
                        {
                            if (it->second <= curr_time)
                            {
                                cb = (*it);
                                for (auto& thread : threads)
                                {
                                    if (thread.is_free())
                                    {
                                        thread.add(cb);
                                        found = true;
                                        break;
                                    }
                                }
                                if (!found)
                                {
                                    break;
                                }
                            }
                            if (found)
                            {
                                it = callbacks->erase(it);
                            }
                            else
                            {
                                ++it;
                            }
                        }
                    } // unlock
                }
            }).detach();
    }

    void signalr_default_scheduler::close()
    {
        (*m_closed) = true;
        m_callback_cv->notify_one();
    }

    signalr_default_scheduler::~signalr_default_scheduler()
    {
        close();
    }
}
