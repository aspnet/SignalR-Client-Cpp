// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#include "stdafx.h"
#include "signalr_default_scheduler.h"
#include <thread>

namespace signalr
{
    thread::thread()
        : m_internals(std::make_shared<internals>())
    {
        auto internals = m_internals;

        std::thread([=]()
            {
                while (internals->m_closed == false)
                {
                    signalr_base_cb cb;
                    {
                        std::unique_lock<std::mutex> lock(internals->m_callback_lock);
                        auto& callbacks = internals->m_callbacks;
                        auto& closed = internals->m_closed;
                        internals->m_callback_cv.wait(lock, [&callbacks, &closed] { return closed || !callbacks.empty(); });
                        if (internals->m_closed)
                        {
                            return;
                        }
                        cb = internals->m_callbacks.front().first;
                        internals->m_callbacks.erase(internals->m_callbacks.begin());
                    } // unlock

                    try
                    {
                        cb();
                    }
                    catch (...)
                    {
                        // ignore exceptions?
                    }
                    internals->m_busy = false;
                }
            }).detach();
    }

    void thread::add(std::pair<signalr_base_cb, std::chrono::time_point<std::chrono::steady_clock, std::chrono::nanoseconds>> cb)
    {
        {
            std::lock_guard<std::mutex> lock(m_internals->m_callback_lock);
            m_internals->m_callbacks.push_back(cb);
            m_internals->m_busy = true;
        } // unlock
        m_internals->m_callback_cv.notify_one();
    }

    void thread::shutdown()
    {
        m_internals->m_closed = true;
        m_internals->m_callback_cv.notify_one();
    }

    bool thread::is_free() const
    {
        return !m_internals->m_busy;
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
            std::lock_guard<std::mutex> lock(m_internals->m_callback_lock);
            m_internals->m_callbacks.push_back(std::make_pair(cb, std::chrono::steady_clock::now() + delay));
        } // unlock
        m_internals->m_callback_cv.notify_one();
    }

    void signalr_default_scheduler::run()
    {
        auto internals = m_internals;

        std::thread([=]()
            {
                std::vector<thread> threads{ 5 };

                while (internals->m_closed == false)
                {
                    std::pair<signalr::signalr_base_cb, std::chrono::time_point<std::chrono::steady_clock, std::chrono::nanoseconds>> cb;
                    {
                        std::unique_lock<std::mutex> lock(internals->m_callback_lock);
                        auto& callbacks = internals->m_callbacks;
                        auto& closed = internals->m_closed;
                        internals->m_callback_cv.wait_for(lock, std::chrono::milliseconds(15), [&callbacks, &closed] { return closed || !callbacks.empty(); });
                        if (closed)
                        {
                            return;
                        }

                        // find the first callback that is ready to run and remove it from the list
                        // then exit the lock so other threads can find a callback
                        auto curr_time = std::chrono::steady_clock::now();
                        auto it = callbacks.begin();
                        auto found = false;
                        while (it != callbacks.end())
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
                                it = callbacks.erase(it);
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
        m_internals->m_closed = true;
        m_internals->m_callback_cv.notify_one();
    }

    signalr_default_scheduler::~signalr_default_scheduler()
    {
        close();
    }
}
