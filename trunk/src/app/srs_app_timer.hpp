/*
The MIT License (MIT)

Copyright (c) 2013-2014 winlin & Baidu Inc.

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#ifndef SRS_APP_TIMER_HPP
#define SRS_APP_TIMER_HPP

#include <srs_app_thread.hpp>

/*
#include <srs_app_timer.hpp>
 */

class SrsTimer {
private:
    int timer_id;
    int click_cnt;
    int interval;
public:
    SrsTimer(int _interval);
    virtual ~SrsTimer();
public:
    void set_timer_id(int timer_id);
    void reset();
    int click();
public:
    virtual void callback();
};

class SrsTimerMgr : public virtual ISrsThreadHandler {
private:
    std::map<int, SrsTimer*> timers;
public:
    SrsTimerMgr();
    ~SrsTimerMgr();
public:
    /**
    * create a timer
    */
    int create_timer(SrsTimer* timer);
    /**
    * remove a timer
    */
    int remove_timer(SrsTimer* timer);

    /**
    * pause a timer
    * that is,
    */
    int pause_timer(SrsTimer* timer);
    int resume_timer(SrsTimer* timer);
};

#endif
