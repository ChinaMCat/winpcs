/*
  Copyright (c) 2015, Ben Cheung (zqzjz1982@gmail.com)
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:
      * Redistributions of source code must retain the above copyright
        notice, this list of conditions and the following disclaimer.
      * Redistributions in binary form must reproduce the above copyright
        notice, this list of conditions and the following disclaimer in the
        documentation and/or other materials provided with the distribution.
      * Neither the name of winpcs nor the
        names of its contributors may be used to endorse or promote products
        derived from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  DISCLAIMED. IN NO EVENT SHALL RANDOLPH VOORHIES AND SHANE GRANT BE LIABLE FOR ANY
  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/*
	timer.hpp for timer generator.
*/
#pragma once

#include "config.hpp"
#include <boost/thread.hpp>
#include <boost/atomic.hpp>


typedef boost::function<void()> functor_type;
typedef boost::asio::deadline_timer timer_t;
typedef boost::shared_ptr<timer_t> timer_ptr_t;
typedef boost::posix_time::seconds second_t;
typedef boost::posix_time::time_duration time_duration_t;

//
//
// timer item define
//
class timer_item : public boost::enable_shared_from_this<timer_item>
{
public:
    template <typename Func>
    timer_item(Func func, boost::asio::io_service& asio_service,
        time_duration_t& interval, bool do_once_immediately) :
        func_(func), interval_(interval), asio_service_(asio_service),
        do_once_immediately_(do_once_immediately)
    {
    }

    time_duration_t interval() const;

    void start();
    void run(const boost::system::error_code &error);
    void cancel();

    functor_type func();

private:
	timer_ptr_t timer_;
	functor_type func_;
	time_duration_t interval_;
	bool do_once_immediately_;
	boost::asio::io_service& asio_service_;
};


class timer_generator : boost::noncopyable
{
public:
	typedef std::map<unsigned long, boost::shared_ptr<timer_item> > timers_container;

    timer_generator();
    ~timer_generator(void);

    int start();

    void stop();

    void kill_timer(unsigned long id);

    // thread function
    void thread_loop();
    void wait();

	//  interface function
    template <typename Func>
    unsigned long set_timer(Func func, long seconds, bool do_once_immediately = false)
    {
        if (stopped_ == 1)
        {
            return 0;
        }

        LOCK lock(mutex_);

        auto timer_item_ptr = boost::make_shared<timer_item>(func, boost::ref(this->asio_service_),
            second_t(seconds), do_once_immediately);

        auto result = timers_.insert(std::make_pair(max_timer_id_++, timer_item_ptr));

        timer_item_ptr->start();
        if (result.second == true)
        {
            return result.first->first;
        }

        return 0;
    }

private:
	MUTEX mutex_;
	boost::atomic_long stopped_;
	boost::shared_ptr<boost::thread> thread_;

	boost::asio::io_service asio_service_;
	boost::shared_ptr<boost::asio::io_service::work> asio_work_;

	unsigned long max_timer_id_;
	timers_container timers_;
};