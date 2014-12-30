/*
   The MIT License (MIT)

   Copyright (c) 2013-2014 winlin

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

#include <srs_app_ping.hpp>
#include <time.h>

SrsPing::SrsPing(SrsRtmpServer* _rtmp) {
    rtmp = _rtmp;
    last_request_time = last_response_time = 0;
    pthread = new SrsThread(this, SRS_PING_INTERVAL*1000*1000LL, true);
}

SrsPing::~SrsPing() {
    srs_freep(pthread);
}

void SrsPing::set_last_response_time(int timestamp) {
    last_response_time = timestamp;
}

int SrsPing::get_res_delay() {
    return (last_request_time - last_response_time) / SRS_PING_INTERVAL;
}

int SrsPing::start() {
    return pthread->start();
}

int SrsPing::cycle() {
    int ret = ERROR_SUCCESS;

    time_t timer = time(NULL);
    if ((int)timer == -1) {
        //something weird happened
        return ret;
    }

    if (rtmp->send_ping_request((int)timer) != ERROR_SUCCESS) {
        // TODO: write some log
        // not return error
        return ret;
    }

    last_request_time = (int)timer;

    srs_trace("send ping request success, last_request_time=%d", last_request_time);

    return ret;
}
