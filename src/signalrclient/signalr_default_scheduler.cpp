// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#include "stdafx.h"
#include "signalr_default_scheduler.h"

namespace signalr
{
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
        if (delay.count() == 0)
        {
            {
                std::lock_guard<std::mutex> lock((*m_callback_lock));
                m_callbacks->push_back(std::make_pair(cb, delay));
            } // unlock
            m_callback_cv->notify_one();
        }
        else
        {
            {
                std::lock_guard<std::mutex> lock((*m_timer_lock));
                m_timer_callbacks->push_back(std::make_pair(cb, delay));
            } // unlock
            m_timer_cv->notify_one();
        }
    }

    void signalr_default_scheduler::run()
    {
        auto closed = m_closed;
        auto callbacks = m_callbacks;
        auto callback_lock = m_callback_lock;
        auto cv = m_callback_cv;
        std::thread([callbacks, callback_lock, cv, closed]()
            {
                std::vector<std::pair<signalr_base_cb, std::chrono::nanoseconds>> tmp;
                while ((*closed) == false)
                {
                    {
                        std::unique_lock<std::mutex> lock((*callback_lock));
                        cv->wait(lock, [&callbacks, &closed] { return (*closed) || !callbacks->empty(); });
                        tmp.swap((*callbacks)); // take all the callbacks while under the lock
                    } // unlock

                    for (auto& cb : tmp)
                    {
                        // todo: dispatch to threadpool
                        std::thread([cb]()
                            {
                                try
                                {
                                    cb.first();
                                }
                                catch (...)
                                {
                                    // ignore exceptions?
                                }
                            }).detach();
                    }

                    tmp.clear();
                }
            }).detach();

            callbacks = m_timer_callbacks;
            callback_lock = m_timer_lock;
            cv = m_timer_cv;
        std::thread([callbacks, callback_lock, cv, closed]()
            {
                std::chrono::steady_clock clock{};
                std::vector<std::pair<signalr_base_cb, std::chrono::nanoseconds>> tmp;
                auto prev_time = clock.now();
                while ((*closed) == false)
                {
                    {
                        std::unique_lock<std::mutex> lock((*callback_lock));
                        cv->wait_for(lock, std::chrono::milliseconds(15), [&closed] { return (*closed); });
                        auto it = callbacks->begin();

                        auto curr_time = clock.now();
                        auto diff = curr_time - prev_time;

                        while (it != callbacks->end())
                        {
                            (*it).second -= diff;
                            if ((*it).second <= std::chrono::milliseconds::zero())
                            {
                                tmp.push_back((*it));
                                it = callbacks->erase(it);
                            }
                            else
                            {
                                ++it;
                            }
                        }
                        prev_time = curr_time;
                    } // unlock

                    for (auto& cb : tmp)
                    {
                        try
                        {
                            cb.first();
                        }
                        catch (...)
                        {
                            // ignore exceptions?
                        }
                    }

                    tmp.clear();
                }
            }).detach();
    }

    void signalr_default_scheduler::close()
    {
        (*m_closed) = true;
        m_callback_cv->notify_one();
        m_timer_cv->notify_one();
    }

    signalr_default_scheduler::~signalr_default_scheduler()
    {
        close();
    }
}
