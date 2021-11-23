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

        m_thread = std::thread([=]()
            {
                while (true)
                {
                    {
                        signalr_base_cb cb;
                        {
                            std::unique_lock<std::mutex> lock(internals->m_callback_lock);
                            auto& callback = internals->m_callback;
                            const auto& closed = internals->m_closed;

                            if (closed && callback == nullptr)
                            {
                                return;
                            }

                            if (callback == nullptr)
                            {
                                internals->m_callback_cv.wait(lock,
                                    [&callback, &closed]
                                    {
                                        return closed || callback != nullptr;
                                    });
                            }

                            if (closed && callback == nullptr)
                            {
                                assert(callback == nullptr);
                                return;
                            }

                            cb = callback;
                            internals->m_callback = nullptr;
                        } // unlock

                        try
                        {
                            cb();
                        }
                        catch (...)
                        {
                            // ignore exceptions?
                            assert(false);
                        }
                    } // destruct cb, otherwise it's possible a shared_ptr is being held by the lambda/function and on destruction it could schedule work which could be
                    // added to this thread and if it then blocks waiting for completion it would deadlock. If we destruct before setting the m_busy flag,
                    // another thread will run the work instead.

                    internals->m_busy = false;
                }

                assert(internals->m_callback == nullptr);
            });
    }

    void thread::add(signalr_base_cb cb)
    {
        {
            assert(m_internals->m_closed == false);
            assert(m_internals->m_busy == false);

            std::lock_guard<std::mutex> lock(m_internals->m_callback_lock);
            m_internals->m_callback = cb;
            m_internals->m_busy = true;
        } // unlock
    }

    void thread::start()
    {
        m_internals->m_callback_cv.notify_one();
    }

    void thread::shutdown()
    {
        {
            std::unique_lock<std::mutex> lock(m_internals->m_callback_lock);
            m_internals->m_closed = true;
        }
        m_internals->m_callback_cv.notify_one();
        m_thread.join();
    }

    bool thread::is_free() const
    {
        return !m_internals->m_busy;
    }

    thread::~thread()
    {
        shutdown();
    }

    void signalr_default_scheduler::schedule(const signalr_base_cb& cb, std::chrono::milliseconds delay)
    {
        {
            std::lock_guard<std::mutex> lock(m_internals->m_callback_lock);
            assert(m_internals->m_closed == false);
            m_internals->m_callbacks.push_back(std::make_pair(cb, std::chrono::steady_clock::now() + delay));
        } // unlock

        // If callback has a delay, no point in running the logic to check if it should run early
        if (delay == std::chrono::milliseconds::zero())
        {
            m_internals->m_callback_cv.notify_one();
        }
    }

    void signalr_default_scheduler::run()
    {
        auto internals = m_internals;

        std::thread([=]()
            {
                std::vector<thread> threads{ 5 };

                std::unique_lock<std::mutex> lock(internals->m_callback_lock);

                auto prev_callback_count = size_t{ 0 };

                while (true)
                {
                    auto& callbacks = internals->m_callbacks;
                    const auto& closed = internals->m_closed;

                    if (closed && callbacks.empty())
                    {
                        return;
                    }

                    internals->m_callback_cv.wait_for(lock, std::chrono::milliseconds(15),
                        [&callbacks, &closed, &prev_callback_count]
                        {
                            return closed || prev_callback_count != callbacks.size();
                        });

                    // find the first callback that is ready to run, find a thread to run it and remove it from the list
                    auto curr_time = std::chrono::steady_clock::now();
                    auto it = callbacks.begin();
                    while (it != callbacks.end())
                    {
                        auto found = false;
                        if (it->second <= curr_time)
                        {
                            for (auto& thread : threads)
                            {
                                if (thread.is_free())
                                {
                                    {
                                        thread.add((*it).first);
                                        (*it).first = nullptr;
                                        // destruct callback in case the destructor can schedule a job which would throw on recursive lock acquisition
                                    }
                                    thread.start();
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

                    prev_callback_count = callbacks.size();
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

    // This will schedule the given func to run every second until the func returns true.
    void timer(const std::shared_ptr<scheduler>& scheduler, std::function<bool(std::chrono::milliseconds)> func)
    {
        timer_internal(scheduler, func, std::chrono::milliseconds::zero());
    }

    void timer_internal(const std::shared_ptr<scheduler>& scheduler, std::function<bool(std::chrono::milliseconds)> func, std::chrono::milliseconds duration)
    {
        constexpr auto tick = std::chrono::seconds(1);
        duration += tick;
        scheduler->schedule([func, scheduler, duration]()
            {
                if (!func(duration))
                {
                    timer_internal(scheduler, func, duration);
                }
            }, tick);
    }
}
