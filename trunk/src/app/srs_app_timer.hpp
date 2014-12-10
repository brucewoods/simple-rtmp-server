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

/*
#include <srs_app_timer.hpp>
 */

#include <srs_core.hpp>

#include <map>

#include <srs_app_thread.hpp>

class SrsServer;

class SrsTimer {
private:
    int timer_id;
    int click_cnt;
    int interval;
    bool paused;
public:
    SrsTimer(int _interval);
    virtual ~SrsTimer();
public:
    int get_timer_id();
    void set_timer_id(int _timer_id);
    void set_interval(int _interval);
    int click();
    bool is_paused();
    /**
    * pause a timer's counting down
    * that is, the timer stop counting down and not calling its callback()
    */
    void pause();
    /**
    * resume a timer's counting down
    * the reverse operation to pause_timer
    */
    void resume();
public:
    virtual void callback() = 0;
};

class SrsTimerMgr : public ISrsThreadHandler {
private:
    const static int TIMER_ID_ALLOC_MASK = 65536;
private:
    int timer_id_alloc;
    std::map<int, SrsTimer*> timers;

    SrsThread* pthread;
    SrsServer* server;
public:
    SrsTimerMgr(SrsServer* srs_server);
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

    /*
    int pause_timer(SrsTimer* timer);
    int resume_timer(SrsTimer* timer);
    */

public:
    int start();
    virtual int cycle();
};

#endif
