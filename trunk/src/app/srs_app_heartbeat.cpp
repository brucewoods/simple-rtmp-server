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

#include <srs_app_heartbeat.hpp>
#include <srs_kernel_error.hpp>

#include <string>
#include <sstream>
using namespace std;

#include <srs_kernel_error.hpp>
#include <srs_kernel_log.hpp>
#include <srs_app_config.hpp>
#include <srs_app_http_client.hpp>
#include <srs_app_json.hpp>
#include <srs_app_http.hpp>
#include <srs_app_utility.hpp>
#include <srs_app_config.hpp>
#include <srs_protocol_rtmp.hpp>
#include <srs_app_tb_http_hooks.hpp>

// conn heartbeat to im srv interval
#define SRS_CONN_HEARTBEAT_INTERVAL_US (int64_t)(9500*1000LL)

SrsConnHeartbeat::SrsConnHeartbeat(SrsRequest* _req, string _ip) {
    req = _req;
    ip = _ip;
    pthread = new SrsThread(this, SRS_CONN_HEARTBEAT_INTERVAL_US, false);
}

SrsConnHeartbeat::~SrsConnHeartbeat() {
    srs_freep(pthread);
}

int SrsConnHeartbeat::cycle() {
    int ret = ERROR_SUCCESS;

    //post heartbeat to im serv
    if (_srs_config->get_vhost_http_hooks_enabled(req->vhost)) {
        // whatever the ret code, notify the api hooks.
        // HTTP: on_heartbeat
        SrsConfDirective* on_heartbeat = _srs_config->get_vhost_on_heartbeat(req->vhost);
        if (!on_heartbeat) {
            srs_info("ignore the empty http callback: on_heartbeat");
            return ret;
        }

        int connection_id = _srs_context->get_id();
        for (int i = 0; i < (int)on_heartbeat->args.size(); i++) {
            std::string url = on_heartbeat->args.at(i);
            SrsTbHttpHooks::on_heartbeat(url, connection_id, ip, req);
        }
    }

    return ret;
}

void SrsConnHeartbeat::callback() {
    pthread->start();
}

