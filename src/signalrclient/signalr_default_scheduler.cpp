// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#include "stdafx.h"
#include <assert.h>
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
                while (true)
                {
                    {
                        signalr_base_cb cb;
                        {
                            std::unique_lock<std::mutex> lock(internals->m_callback_lock);
                            auto& callbacks = internals->m_callbacks;
                            auto& closed = internals->m_closed;
                            internals->m_callback_cv.wait(lock, [&callbacks, &closed] { return closed || !callbacks.empty(); });
                            if (internals->m_closed && callbacks.empty())
                            {
                                assert(callbacks.empty());
                                return;
                            }
                            cb = internals->m_callbacks.front();
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
                    } // destruct cb, otherwise it's possible a shared_ptr is being held by the lambda/function and on destruction it could schedule work which could be
                    // added to this thread and if it then blocks waiting for completion it would deadlock. If we destruct before setting the m_busy flag,
                    // another thread will run the work instead.

                    internals->m_busy = false;
                }

                assert(internals->m_callbacks.empty());
            }).detach();
    }

    void thread::add(signalr_base_cb cb)
    {
        {
            assert(m_internals->m_closed == false);
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
            assert(m_internals->m_closed == false);
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

                while (true)
                {
                    signalr::signalr_base_cb cb;
                    {
                        std::unique_lock<std::mutex> lock(internals->m_callback_lock);
                        auto& callbacks = internals->m_callbacks;
                        auto& closed = internals->m_closed;
                        internals->m_callback_cv.wait_for(lock, std::chrono::milliseconds(15), [&callbacks, &closed] { return closed || !callbacks.empty(); });

                        if (closed && callbacks.empty())
                        {
                            assert(callbacks.empty());
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
                                cb = (*it).first;
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

                assert(internals->m_callbacks.empty());
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
