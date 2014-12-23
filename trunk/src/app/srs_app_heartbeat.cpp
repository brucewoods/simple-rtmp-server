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

#ifdef SRS_AUTO_HTTP_PARSER

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

SrsHttpHeartbeat::SrsHttpHeartbeat()
{
}

SrsHttpHeartbeat::~SrsHttpHeartbeat()
{
    srs_freep(pthread);
}

void SrsHttpHeartbeat::heartbeat()
{
    int ret = ERROR_SUCCESS;
    
    std::string url = _srs_config->get_heartbeat_url();
    
    SrsHttpUri uri;
    if ((ret = uri.initialize(url)) != ERROR_SUCCESS) {
        srs_error("http uri parse hartbeart url failed. url=%s, ret=%d", url.c_str(), ret);
        return;
    }

    std::string ip = "";
    std::string device_id = _srs_config->get_heartbeat_device_id();
    
    vector<string>& ips = srs_get_local_ipv4_ips();
    if (!ips.empty()) {
        ip = ips[_srs_config->get_stats_network() % (int)ips.size()];
    }
    
    std::stringstream ss;
    ss << __SRS_JOBJECT_START
        << __SRS_JFIELD_STR("device_id", device_id) << __SRS_JFIELD_CONT
        << __SRS_JFIELD_STR("ip", ip);
    if (_srs_config->get_heartbeat_summaries()) {
        ss << __SRS_JFIELD_CONT << __SRS_JFIELD_ORG("summaries", "");
        srs_api_dump_summaries(ss);
    }
    ss << __SRS_JOBJECT_END;
    std::string data = ss.str();
    std::string res;
    
    SrsHttpClient http;
    if ((ret = http.post(&uri, data, res)) != ERROR_SUCCESS) {
        srs_info("http post hartbeart uri failed. "
            "url=%s, request=%s, response=%s, ret=%d",
            url.c_str(), data.c_str(), res.c_str(), ret);
        return;
    }
    
    srs_info("http hook hartbeart success. "
        "url=%s, request=%s, response=%s, ret=%d",
        url.c_str(), data.c_str(), res.c_str(), ret);
    
    return;
}

SrsConnHeartbeat::SrsConnHeartbeat(int _interval, SrsRequest* _req, string _ip) : SrsTimer(_interval) {
    req = _req;
    ip = _ip;
    pthread = NULL;
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

    pthread->stop_loop();
    return ret;
}

void SrsConnHeartbeat::callback() {
    srs_freep(pthread);
    pthread = new SrsThread(this, 0LL, false);
    pthread->start();
}

#endif

