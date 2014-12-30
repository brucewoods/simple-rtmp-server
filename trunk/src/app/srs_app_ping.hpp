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

#ifndef SRS_APP_PING_HPP
#define SRS_APP_PING_HPP

/*
#include <srs_app_ping.hpp>
*/

#include <srs_core.hpp>

#include <srs_app_thread.hpp>
#include <srs_protocol_rtmp.hpp>

class SrsPing: public ISrsThreadHandler {
public:
    static const int SRS_PING_INTERVAL = 10;
    //i.e. if a pingrequest was sent
private:
    SrsThread* pthread;
    int last_request_time;
    int last_response_time;
    SrsRtmpServer* rtmp;
public:
    SrsPing(SrsRtmpServer* _rtmp);
    virtual ~SrsPing();
public:
    void set_last_response_time(int timestamp);
    /*
    if a negative value returned, the client is considered encounting problems,
    the app may close the connection
     */
    int get_res_delay();
    int start();
public:
    virtual int cycle();
};

#endif

