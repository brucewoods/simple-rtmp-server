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

#include <srs_app_tb_http_hooks.hpp>

#ifdef SRS_AUTO_HTTP_CALLBACK

#include <srs_kernel_error.hpp>
#include <srs_protocol_rtmp.hpp>
#include <srs_app_st_socket.hpp>
#include <srs_app_http.hpp>
#include <srs_app_json.hpp>
#include <srs_app_http_client.hpp>

#define TB_IM_METHOD_CHECK_USER_INFO "checkUserInfo"
#define TB_IM_METHOD_NOTIFY_STREAM_STATUS "notifyStreamStatus"
#define TB_IM_CMD_CHECK_USER_INFO "107120"
#define TB_IM_CMD_NOTIFY_STREAM_STATUS "107121"

SrsTbHttpHooks::SrsTbHttpHooks()
{
}

SrsTbHttpHooks::~SrsTbHttpHooks()
{
}

template<class T>
void SrsTbHttpHooks::append_param(std::stringstream& s, const char* key, const T& value, bool with_amp) {
    s << key << "=" << value;
    if (with_amp) {
        s << "&";
    }
}

int SrsTbHttpHooks::on_connect(string url, int client_id, string ip, SrsRequest* req)
{
    int ret = ERROR_SUCCESS;

    SrsHttpUri uri;
    if ((ret = uri.initialize(url)) != ERROR_SUCCESS) {
        srs_error("http uri parse on_connect url failed. "
            "client_id=%d, url=%s, ret=%d", client_id, url.c_str(), ret);
        return ret;
    }

    std::stringstream ss;
    append_param(ss, "method", TB_IM_METHOD_CHECK_USER_INFO);
    append_param(ss, "cmd", TB_IM_CMD_CHECK_USER_INFO);
    append_param(ss, "group_id", req->client_info->group_id);
    append_param(ss, "user_id", req->client_info->user_id);
    append_param("identity", "publisher", true);
    append_param("publishToken", req->publish_token);
    append_param("client_type", req->client_type);
    append_param("net_type", 0, false);
    std::string data = ss.str();
    std::string res;

    SrsHttpClient http;
    if ((ret = http.post(&uri, data, res)) != ERROR_SUCCESS) {
        srs_error("http post on_connect uri failed. "
            "client_id=%d, url=%s, request=%s, response=%s, ret=%d",
            client_id, url.c_str(), data.c_str(), res.c_str(), ret);
        return ret;
    }

    /*
    if (res.empty() || res != SRS_HTTP_RESPONSE_OK) {
        ret = ERROR_HTTP_DATA_INVLIAD;
        srs_error("http hook on_connect validate failed. "
            "client_id=%d, res=%s, ret=%d", client_id, res.c_str(), ret);
        return ret;
    }
    */

    srs_trace("http hook on_connect success. "
        "client_id=%d, url=%s, request=%s, response=%s, ret=%d",
        client_id, url.c_str(), data.c_str(), res.c_str(), ret);

    return ret;
}


#endif
