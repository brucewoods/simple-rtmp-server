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

#include <srs_kernel_error.hpp>
#include <srs_protocol_rtmp.hpp>
#include <srs_app_st_socket.hpp>
#include <srs_app_http.hpp>
#include <srs_app_json.hpp>
#include <srs_app_http_client.hpp>
#include <srs_app_tb_log.hpp>

using namespace std;

#define TB_CLIVE_METHOD_CHECK_USER_INFO "checkUserInfo"
#define TB_CLIVE_METHOD_NOTIFY_STREAM_STATUS "notifyStreamStatus"
#define TB_CLIVE_CMD_CHECK_USER_INFO "107120"
#define TB_CLIVE_CMD_NOTIFY_STREAM_STATUS "107121"

#define TB_CLIVE_STATUS_START 1
#define TB_CLIVE_STATUS_CLOSE 6
#define TB_CLIVE_STATUS_ERRORCLOSE 8
#define TB_CLIVE_STATUS_PUBLISH_PAUSE 2
#define TB_CLIVE_STATUS_PUBLISH_RESUME 9
#define TB_CLIVE_STATUS_HEARTBEAT 10

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

int SrsTbHttpHooks::get_res_data(const string &res, int& error, SrsJsonObject*& http_res, SrsJsonObject*& data) {
    int ret = ERROR_SUCCESS;

    if (res.empty()) {
        ret = ERROR_HTTP_DATA_INVLIAD;
        srs_error("http hook on_publish validate failed. "
            "res=%s, ret=%d", res.c_str(), ret);
        return ret;
    }

    char *res_str = new char[res.length()];
    strncpy(res_str, res.c_str(), res.length());

    SrsJsonAny* http_res_tmp = NULL;
    if (!(http_res_tmp = SrsJsonAny::loads(res_str)))  {
        ret = ERROR_HTTP_DATA_INVLIAD;
        srs_error("http hook on_publish json parse failed. "
            "res=%s, ret=%d", res.c_str(), ret);
        srs_freep(http_res_tmp);
        srs_freep(res_str);
        return ret;
    }
    srs_freep(res_str);

    if (!http_res_tmp->is_object()) {
        ret = ERROR_HTTP_DATA_INVLIAD;
        srs_error("http hook on_publish json not an object. "
            "res=%s, ret=%d", res.c_str(), ret);
        srs_freep(http_res_tmp);
        return ret;
    }
    http_res = http_res_tmp->to_object();

    SrsJsonAny* _error = http_res->get_property("error");
    if (!_error || !_error->is_integer()) {
        ret = ERROR_HTTP_DATA_INVLIAD;
        srs_error("http hook on_publish no error field. "
            "res=%s, ret=%d", res.c_str(), ret);
        //srs_freep(http_res_tmp);
        return ret;
    }
    error = (int)(_error->to_integer());

    SrsJsonAny* _data = http_res->get_property("data");
    if (!_data || !_data->is_object()) { // ok, no data field
        data = NULL;
    } else {
        data = _data->to_object();
    }

    return ret;
}

int SrsTbHttpHooks::on_connect(string url, int client_id, string ip, SrsRequest* req) {
    int ret = ERROR_SUCCESS;

    SrsHttpUri uri;
    if ((ret = uri.initialize(url)) != ERROR_SUCCESS) {
        srs_error("http uri parse on_connect url failed. "
            "client_id=%d, url=%s, ret=%d", client_id, url.c_str(), ret);
        _tb_log->conn_log(TbLogLevel::Error, LOGTYPE_HOOK, req, "action=connect url=%s file=%s line=%d errno=%d errmsg=parse_on_connect_url_failed", url.c_str(), __FILE__, __LINE__, ret);
        return ret;
    }

    srs_assert(req->client_info);

    std::stringstream ss;
    append_param(ss, "method", TB_CLIVE_METHOD_CHECK_USER_INFO);
    append_param(ss, "cmd", TB_CLIVE_CMD_CHECK_USER_INFO);
    append_param(ss, "groupId", req->client_info->group_id);
    append_param(ss, "userId", req->client_info->user_id);
    append_param(ss, "identity", req->client_info->user_role);
    append_param(ss, "publishToken", "test");
    append_param(ss, "client_type", req->client_info->client_type);
    append_param(ss, "net_type", 0, false);
    std::string postdata = ss.str();
    std::string res;

    SrsHttpClient http;
    if ((ret = http.post(&uri, postdata, res)) != ERROR_SUCCESS) {
        srs_error("http post on_connect uri failed. "
            "client_id=%d, url=%s, request=%s, response=%s, ret=%d",
            client_id, url.c_str(), postdata.c_str(), res.c_str(), ret);
        _tb_log->conn_log(TbLogLevel::Error, LOGTYPE_HOOK, req, "action=connect url=%s file=%s line=%d errno=%d errmsg=post_on_connect_url_failed", url.c_str(), __FILE__, __LINE__, ret);
        return ret;
    }

    int error = 0;
    SrsJsonObject* http_res = NULL;
    SrsJsonObject* data = NULL;

    try {
        if (get_res_data(res, error, http_res, data) != ERROR_SUCCESS) {
            srs_error("http post on_connect parse result failed. "
                    "client_id=%d, url=%s, request=%s, response=%s, ret=%d",
                    client_id, url.c_str(), postdata.c_str(), res.c_str(), ret);
            _tb_log->conn_log(TbLogLevel::Error, LOGTYPE_HOOK, req, "action=connect url=%s file=%s line=%d errno=%d errmsg=get_res_data_failed", url.c_str(), __FILE__, __LINE__, ERROR_HTTP_DATA_INVLIAD);
            throw ERROR_HTTP_DATA_INVLIAD;
        }
        if (error != 0) {
            srs_error("http post on_connect error non zero. "
                    "client_id=%d, url=%s, request=%s, response=%s, ret=%d",
                    client_id, url.c_str(), postdata.c_str(), res.c_str(), ret);
            _tb_log->conn_log(TbLogLevel::Error, LOGTYPE_HOOK, req, "action=connect url=%s file=%s line=%d errno=%d errmsg=get_res_data_failed", url.c_str(), __FILE__, __LINE__, ERROR_HTTP_ERROR_RETURNED);
            throw ERROR_HTTP_ERROR_RETURNED;
        }
        SrsJsonAny *accept = NULL;
        if (error != 0 || !(accept = data->get_property("accept")) || !accept->is_integer()) {
            srs_error("http post on_connect parse result failed. "
                    "client_id=%d, url=%s, request=%s, response=%s, ret=%d",
                    client_id, url.c_str(), postdata.c_str(), res.c_str(), ret);
            _tb_log->conn_log(TbLogLevel::Error, LOGTYPE_HOOK, req, "action=connect url=%s file=%s line=%d errno=%d errmsg=parse_result_failed", url.c_str(), __FILE__, __LINE__, ERROR_HTTP_DATA_INVLIAD);
            throw ERROR_HTTP_DATA_INVLIAD;
        }
        if (accept->to_integer() == 0LL) {
            srs_error("http post on_connect authentification check failed. "
                    "client_id=%d, url=%s, request=%s, response=%s, ret=%d",
                    client_id, url.c_str(), postdata.c_str(), res.c_str(), ret);
            _tb_log->conn_log(TbLogLevel::Error, LOGTYPE_HOOK, req, "action=connect url=%s file=%s line=%d errno=%d errmsg=authentification_check_failed", url.c_str(), __FILE__, __LINE__, ERROR_HTTP_ON_CONNECT_AUTH_FAIL);
            throw ERROR_HTTP_ON_CONNECT_AUTH_FAIL;
        }
    } catch (int r) {
        srs_freep(http_res);
        return (ret = r);
    }

    SrsJsonAny* net_type = NULL;
    if ((net_type = data->get_property("net_type")) && net_type->is_integer()) {
        // TODO: write net_type to req
        req->client_info->net_type = net_type->to_integer();
    } else {
        // TODO: write log

    }

    srs_freep(http_res);

    srs_trace("http hook on_connect success. "
            "client_id=%d, url=%s, request=%s, response=%s, ret=%d",
            client_id, url.c_str(), postdata.c_str(), res.c_str(), ret);

    return ret;
}

int SrsTbHttpHooks::on_publish(string url, int client_id, string ip, SrsRequest* req) {
    int ret = ERROR_SUCCESS;

    SrsHttpUri uri;
    if ((ret = uri.initialize(url)) != ERROR_SUCCESS) {
        srs_error("http uri parse on_publish2 url failed. "
                "client_id=%d, url=%s, ret=%d", client_id, url.c_str(), ret);
        _tb_log->conn_log(TbLogLevel::Error, LOGTYPE_HOOK, req, "action=publish url=%s file=%s line=%d errno=%d errmsg=parse_url_failed", url.c_str(), __FILE__, __LINE__, ret);
        
        return ret;
    }

    srs_assert(req->client_info);

    std::stringstream ss;
    append_param(ss, "method", TB_CLIVE_METHOD_NOTIFY_STREAM_STATUS);
    append_param(ss, "cmd", TB_CLIVE_CMD_NOTIFY_STREAM_STATUS);
    append_param(ss, "groupId", req->client_info->group_id);
    append_param(ss, "userId", req->client_info->user_id);
    append_param(ss, "identity", req->client_info->user_role);
    append_param(ss, "status", TB_CLIVE_STATUS_START, false);
    std::string postdata = ss.str();
    std::string res;

    SrsHttpClient http;
    if ((ret = http.post(&uri, postdata, res)) != ERROR_SUCCESS) {
        srs_error("http post on_publish2 uri failed. "
                "client_id=%d, url=%s, request=%s, response=%s, ret=%d",
                client_id, url.c_str(), postdata.c_str(), res.c_str(), ret);
        _tb_log->conn_log(TbLogLevel::Error, LOGTYPE_HOOK, req, "action=publish url=%s file=%s line=%d errno=%d errmsg=post_url_failed", url.c_str(), __FILE__, __LINE__, ret);
        
        return ret;
    }

    int error = 0;
    SrsJsonObject* data = NULL;
    SrsJsonObject* http_res = NULL;

    try {
        if (get_res_data(res, error, http_res, data) != ERROR_SUCCESS) {
            srs_error("http post on_publish2 parse result failed. "
                    "client_id=%d, url=%s, request=%s, response=%s, ret=%d",
                    client_id, url.c_str(), postdata.c_str(), res.c_str(), ret);
            _tb_log->conn_log(TbLogLevel::Error, LOGTYPE_HOOK, req, "action=publish url=%s file=%s line=%d errno=%d errmsg=get_res_data_failed", url.c_str(), __FILE__, __LINE__, ERROR_HTTP_DATA_INVLIAD);
        
            throw ERROR_HTTP_DATA_INVLIAD;
        }
        if (error != 0) {
            srs_error("http post on_publish2 error non zero. "
                    "client_id=%d, url=%s, request=%s, response=%s, ret=%d",
                    client_id, url.c_str(), postdata.c_str(), res.c_str(), ret);
            _tb_log->conn_log(TbLogLevel::Error, LOGTYPE_HOOK, req, "action=publish url=%s file=%s line=%d errno=%d errmsg=error_non_zero", url.c_str(), __FILE__, __LINE__, ERROR_HTTP_ERROR_RETURNED);
        
            throw ERROR_HTTP_ERROR_RETURNED;
        }
    } catch (int r) {
        srs_freep(http_res);
        return (ret = r);
    }

    srs_freep(http_res);
    srs_trace("http hook on_publish success. "
            "client_id=%d, url=%s, request=%s, response=%s, ret=%d",
            client_id, url.c_str(), postdata.c_str(), res.c_str(), ret);

    return ret;
}

int SrsTbHttpHooks::on_unpublish(string url, int client_id, string ip, SrsRequest* req) {
    int ret = ERROR_SUCCESS;

    SrsHttpUri uri;
    if ((ret = uri.initialize(url)) != ERROR_SUCCESS) {
        srs_error("http uri parse on_unpublish url failed. "
                "client_id=%d, url=%s, ret=%d", client_id, url.c_str(), ret);
        _tb_log->conn_log(TbLogLevel::Error, LOGTYPE_HOOK, req, "action=unpublish url=%s file=%s line=%d errno=%d errmsg=parse_url_failed", url.c_str(), __FILE__, __LINE__, ret);
        
        return ret;
    }

    srs_assert(req->client_info);

    std::stringstream ss;
    append_param(ss, "method", TB_CLIVE_METHOD_NOTIFY_STREAM_STATUS);
    append_param(ss, "cmd", TB_CLIVE_CMD_NOTIFY_STREAM_STATUS);
    append_param(ss, "groupId", req->client_info->group_id);
    append_param(ss, "userId", req->client_info->user_id);
    append_param(ss, "identity", req->client_info->user_role);
    append_param(ss, "status", TB_CLIVE_STATUS_CLOSE, false);
    std::string postdata = ss.str();
    std::string res;

    SrsHttpClient http;
    if ((ret = http.post(&uri, postdata, res)) != ERROR_SUCCESS) {
        srs_error("http post on_unpublish uri failed. "
                "client_id=%d, url=%s, request=%s, response=%s, ret=%d",
                client_id, url.c_str(), postdata.c_str(), res.c_str(), ret);
        _tb_log->conn_log(TbLogLevel::Error, LOGTYPE_HOOK, req, "action=unpublish url=%s file=%s line=%d errno=%d errmsg=post_url_failed", url.c_str(), __FILE__, __LINE__, ret);
   
        return ret;
    }

    int error = 0;
    SrsJsonObject* data = NULL;
    SrsJsonObject* http_res = NULL;

    try {
        if (get_res_data(res, error, http_res, data) != ERROR_SUCCESS) {
            srs_error("http post on_unpublish parse result failed. "
                    "client_id=%d, url=%s, request=%s, response=%s, ret=%d",
                    client_id, url.c_str(), postdata.c_str(), res.c_str(), ret);
            _tb_log->conn_log(TbLogLevel::Error, LOGTYPE_HOOK, req, "action=unpublish url=%s file=%s line=%d errno=%d errmsg=get_res_data_failed", url.c_str(), __FILE__, __LINE__, ERROR_HTTP_DATA_INVLIAD);
   
            throw ERROR_HTTP_DATA_INVLIAD;
        }
        if (error != 0) {
            srs_error("http post on_unpublish error non zero. "
                    "client_id=%d, url=%s, request=%s, response=%s, ret=%d",
                    client_id, url.c_str(), postdata.c_str(), res.c_str(), ret);
            _tb_log->conn_log(TbLogLevel::Error, LOGTYPE_HOOK, req, "action=unpublish url=%s file=%s line=%d errno=%d errmsg=error_non_zero", url.c_str(), __FILE__, __LINE__, ERROR_HTTP_ERROR_RETURNED);
   
            throw ERROR_HTTP_ERROR_RETURNED;
        }
    } catch (int r) {
        srs_freep(http_res);
        return (ret = r);
    }

    srs_freep(http_res);
    srs_trace("http hook on_unpublish success. "
            "client_id=%d, url=%s, request=%s, response=%s, ret=%d",
            client_id, url.c_str(), postdata.c_str(), res.c_str(), ret);

    return ret;
}

int SrsTbHttpHooks::on_errorclose(string url, int client_id, string ip, SrsRequest* req) {
    int ret = ERROR_SUCCESS;

    SrsHttpUri uri;
    if ((ret = uri.initialize(url)) != ERROR_SUCCESS) {
        srs_error("http uri parse on_error_close url failed. "
                "client_id=%d, url=%s, ret=%d", client_id, url.c_str(), ret);
        _tb_log->conn_log(TbLogLevel::Error, LOGTYPE_HOOK, req, "action=errclose url=%s file=%s line=%d errno=%d errmsg=parse_url_failed", url.c_str(), __FILE__, __LINE__, ret);
   
        return ret;
    }

    srs_assert(req->client_info);

    std::stringstream ss;
    append_param(ss, "method", TB_CLIVE_METHOD_NOTIFY_STREAM_STATUS);
    append_param(ss, "cmd", TB_CLIVE_CMD_NOTIFY_STREAM_STATUS);
    append_param(ss, "groupId", req->client_info->group_id);
    append_param(ss, "userId", req->client_info->user_id);
    append_param(ss, "identity", req->client_info->user_role);
    append_param(ss, "status", TB_CLIVE_STATUS_ERRORCLOSE, false);
    std::string postdata = ss.str();
    std::string res;

    SrsHttpClient http;
    if ((ret = http.post(&uri, postdata, res)) != ERROR_SUCCESS) {
        srs_error("http post on_error_close uri failed. "
                "client_id=%d, url=%s, request=%s, response=%s, ret=%d",
                client_id, url.c_str(), postdata.c_str(), res.c_str(), ret);
        _tb_log->conn_log(TbLogLevel::Error, LOGTYPE_HOOK, req, "action=errclose url=%s file=%s line=%d errno=%d errmsg=post_url_failed", url.c_str(), __FILE__, __LINE__, ret);
        return ret;
    }

    int error = 0;
    SrsJsonObject* data = NULL;
    SrsJsonObject* http_res = NULL;

    try {
        if (get_res_data(res, error, http_res, data) != ERROR_SUCCESS) {
            srs_error("http post on_error_close parse result failed. "
                    "client_id=%d, url=%s, request=%s, response=%s, ret=%d",
                    client_id, url.c_str(), postdata.c_str(), res.c_str(), ret);
            _tb_log->conn_log(TbLogLevel::Error, LOGTYPE_HOOK, req, "action=errclose url=%s file=%s line=%d errno=%d errmsg=get_res_data_failed", url.c_str(), __FILE__, __LINE__, ERROR_HTTP_DATA_INVLIAD);
       
            throw ERROR_HTTP_DATA_INVLIAD;
        }
        if (error != 0) {
            srs_error("http post on_error_close error non zero. "
                    "client_id=%d, url=%s, request=%s, response=%s, ret=%d",
                    client_id, url.c_str(), postdata.c_str(), res.c_str(), ret);
            _tb_log->conn_log(TbLogLevel::Error, LOGTYPE_HOOK, req, "action=errclose url=%s file=%s line=%d errno=%d errmsg=error_non_zero", url.c_str(), __FILE__, __LINE__, ERROR_HTTP_ERROR_RETURNED);
       
            throw ERROR_HTTP_ERROR_RETURNED;
        }
    } catch (int r) {
        srs_freep(http_res);
        return (ret = r);
    }

    srs_freep(http_res);
    srs_trace("http hook on_error_close success. "
            "client_id=%d, url=%s, request=%s, response=%s, ret=%d",
            client_id, url.c_str(), postdata.c_str(), res.c_str(), ret);

    return ret;
}

int SrsTbHttpHooks::on_publish_pause(string url, int client_id, string ip, SrsRequest* req) {
    int ret = ERROR_SUCCESS;

    SrsHttpUri uri;
    if ((ret = uri.initialize(url)) != ERROR_SUCCESS) {
        srs_error("http uri parse on_publish_pause url failed. "
                "client_id=%d, url=%s, ret=%d", client_id, url.c_str(), ret);
        _tb_log->conn_log(TbLogLevel::Error, LOGTYPE_HOOK, req, "action=publish_pause url=%s file=%s line=%d errno=%d errmsg=parse_url_failed", url.c_str(), __FILE__, __LINE__, ret);
       
        return ret;
    }

    srs_assert(req->client_info);

    std::stringstream ss;
    append_param(ss, "method", TB_CLIVE_METHOD_NOTIFY_STREAM_STATUS);
    append_param(ss, "cmd", TB_CLIVE_CMD_NOTIFY_STREAM_STATUS);
    append_param(ss, "groupId", req->client_info->group_id);
    append_param(ss, "userId", req->client_info->user_id);
    append_param(ss, "identity", req->client_info->user_role);
    append_param(ss, "status", TB_CLIVE_STATUS_PUBLISH_PAUSE, false);
    std::string postdata = ss.str();
    std::string res;

    SrsHttpClient http;
    if ((ret = http.post(&uri, postdata, res)) != ERROR_SUCCESS) {
        srs_error("http post on_publish_pause uri failed. "
                "client_id=%d, url=%s, request=%s, response=%s, ret=%d",
                client_id, url.c_str(), postdata.c_str(), res.c_str(), ret);
        _tb_log->conn_log(TbLogLevel::Error, LOGTYPE_HOOK, req, "action=publish_pause url=%s file=%s line=%d errno=%d errmsg=post_url_failed", url.c_str(), __FILE__, __LINE__, ret);
       
        return ret;
    }

    int error = 0;
    SrsJsonObject* data = NULL;
    SrsJsonObject* http_res = NULL;

    try {
        if (get_res_data(res, error, http_res, data) != ERROR_SUCCESS) {
            srs_error("http post on_publish_pause parse result failed. "
                    "client_id=%d, url=%s, request=%s, response=%s, ret=%d",
                    client_id, url.c_str(), postdata.c_str(), res.c_str(), ret);
            _tb_log->conn_log(TbLogLevel::Error, LOGTYPE_HOOK, req, "action=publish_pause url=%s file=%s line=%d errno=%d errmsg=get_res_data_failed", url.c_str(), __FILE__, __LINE__, ERROR_HTTP_DATA_INVLIAD);
 
            throw ERROR_HTTP_DATA_INVLIAD;
        }
        if (error != 0) {
            srs_error("http post on_publish_pause error non zero. "
                    "client_id=%d, url=%s, request=%s, response=%s, ret=%d",
                    client_id, url.c_str(), postdata.c_str(), res.c_str(), ret);
            _tb_log->conn_log(TbLogLevel::Error, LOGTYPE_HOOK, req, "action=publish_pause url=%s file=%s line=%d errno=%d errmsg=error_non_zero", url.c_str(), __FILE__, __LINE__, ERROR_HTTP_ERROR_RETURNED);
 
            throw ERROR_HTTP_ERROR_RETURNED;
        }
    } catch (int r) {
        srs_freep(http_res);
        return (ret = r);
    }

    srs_freep(http_res);
    srs_trace("http hook on_publish_pause success. "
            "client_id=%d, url=%s, request=%s, response=%s, ret=%d",
            client_id, url.c_str(), postdata.c_str(), res.c_str(), ret);

    return ret;
}

int SrsTbHttpHooks::on_publish_resume(string url, int client_id, string ip, SrsRequest* req) {
    int ret = ERROR_SUCCESS;

    SrsHttpUri uri;
    if ((ret = uri.initialize(url)) != ERROR_SUCCESS) {
        srs_error("http uri parse on_publish_resume url failed. "
                "client_id=%d, url=%s, ret=%d", client_id, url.c_str(), ret);
        _tb_log->conn_log(TbLogLevel::Error, LOGTYPE_HOOK, req, "action=publish_resume url=%s file=%s line=%d errno=%d errmsg=parse_url_failed", url.c_str(), __FILE__, __LINE__, ret);
 
        return ret;
    }

    srs_assert(req->client_info);

    std::stringstream ss;
    append_param(ss, "method", TB_CLIVE_METHOD_NOTIFY_STREAM_STATUS);
    append_param(ss, "cmd", TB_CLIVE_CMD_NOTIFY_STREAM_STATUS);
    append_param(ss, "groupId", req->client_info->group_id);
    append_param(ss, "userId", req->client_info->user_id);
    append_param(ss, "identity", req->client_info->user_role);
    append_param(ss, "status", TB_CLIVE_STATUS_PUBLISH_RESUME, false);
    std::string postdata = ss.str();
    std::string res;

    SrsHttpClient http;
    if ((ret = http.post(&uri, postdata, res)) != ERROR_SUCCESS) {
        srs_error("http post on_publish_resume uri failed. "
                "client_id=%d, url=%s, request=%s, response=%s, ret=%d",
                client_id, url.c_str(), postdata.c_str(), res.c_str(), ret);
        _tb_log->conn_log(TbLogLevel::Error, LOGTYPE_HOOK, req, "action=publish_resume url=%s file=%s line=%d errno=%d errmsg=post_url_failed", url.c_str(), __FILE__, __LINE__, ret);
 
        return ret;
    }

    int error = 0;
    SrsJsonObject* data = NULL;
    SrsJsonObject* http_res = NULL;

    try {
        if (get_res_data(res, error, http_res, data) != ERROR_SUCCESS) {
            srs_error("http post on_publish_resume parse result failed. "
                    "client_id=%d, url=%s, request=%s, response=%s, ret=%d",
                    client_id, url.c_str(), postdata.c_str(), res.c_str(), ret);
            _tb_log->conn_log(TbLogLevel::Error, LOGTYPE_HOOK, req, "action=publish_resume url=%s file=%s line=%d errno=%d errmsg=get_res_data_failed", url.c_str(), __FILE__, __LINE__, ERROR_HTTP_DATA_INVLIAD);
 
            throw ERROR_HTTP_DATA_INVLIAD;
        }
        if (error != 0) {
            srs_error("http post on_publish_resume error non zero. "
                    "client_id=%d, url=%s, request=%s, response=%s, ret=%d",
                    client_id, url.c_str(), postdata.c_str(), res.c_str(), ret);
            _tb_log->conn_log(TbLogLevel::Error, LOGTYPE_HOOK, req, "action=publish_resume url=%s file=%s line=%d errno=%d errmsg=error_non_zero", url.c_str(), __FILE__, __LINE__, ret);
 
            throw ERROR_HTTP_ERROR_RETURNED;
        }
    } catch (int r) {
        srs_freep(http_res);
        return (ret = r);
    }

    srs_freep(http_res);
    srs_trace("http hook on_publish_resume success. "
            "client_id=%d, url=%s, request=%s, response=%s, ret=%d",
            client_id, url.c_str(), postdata.c_str(), res.c_str(), ret);

    return ret;
}

int SrsTbHttpHooks::on_heartbeat(string url, int client_id, string ip, SrsRequest* req) {
    int ret = ERROR_SUCCESS;

    SrsHttpUri uri;
    if ((ret = uri.initialize(url)) != ERROR_SUCCESS) {
        srs_error("http uri parse on_heartbeat url failed. "
                "client_id=%d, url=%s, ret=%d", client_id, url.c_str(), ret);
		_tb_log->conn_log(TbLogLevel::Error, LOGTYPE_HOOK, req, "action=heart_beat url=%s file=%s line=%d errno=%d errmsg=parse_url_failed", url.c_str(), __FILE__, __LINE__, ret);
        return ret;
    }

    srs_assert(req->client_info);

    std::stringstream ss;
    append_param(ss, "method", TB_CLIVE_METHOD_NOTIFY_STREAM_STATUS);
    append_param(ss, "cmd", TB_CLIVE_CMD_NOTIFY_STREAM_STATUS);
    append_param(ss, "groupId", req->client_info->group_id);
    append_param(ss, "userId", req->client_info->user_id);
    append_param(ss, "identity", req->client_info->user_role);
    append_param(ss, "status", TB_CLIVE_STATUS_HEARTBEAT, false);
    std::string postdata = ss.str();
    std::string res;

    SrsHttpClient http;
    if ((ret = http.post(&uri, postdata, res)) != ERROR_SUCCESS) {
        srs_error("http post on_heartbeat uri failed. "
                "client_id=%d, url=%s, request=%s, response=%s, ret=%d",
                client_id, url.c_str(), postdata.c_str(), res.c_str(), ret);
		_tb_log->conn_log(TbLogLevel::Error, LOGTYPE_HOOK, req, "action=heart_beat url=%s file=%s line=%d errno=%d errmsg=post_url_failed", url.c_str(), __FILE__, __LINE__, ret);
		return ret;
    }

    int error = 0;
    SrsJsonObject* data = NULL;
    SrsJsonObject* http_res = NULL;

    try {
        if (get_res_data(res, error, http_res, data) != ERROR_SUCCESS) {
            srs_error("http post on_heartbeat parse result failed. "
                    "client_id=%d, url=%s, request=%s, response=%s, ret=%d",
                    client_id, url.c_str(), postdata.c_str(), res.c_str(), ret);
			_tb_log->conn_log(TbLogLevel::Error, LOGTYPE_HOOK, req, "action=heart_beat url=%s file=%s line=%d errno=%d errmsg=get_res_data_failed", url.c_str(), __FILE__, __LINE__, ERROR_HTTP_DATA_INVLIAD);
            throw ERROR_HTTP_DATA_INVLIAD;
        }
        if (error != 0) {
            srs_error("http post on_heartbeat error non zero. "
                    "client_id=%d, url=%s, request=%s, response=%s, ret=%d",
                    client_id, url.c_str(), postdata.c_str(), res.c_str(), ret);
			_tb_log->conn_log(TbLogLevel::Error, LOGTYPE_HOOK, req, "action=heart_beat url=%s file=%s line=%d errno=%d errmsg=error_non_zero", url.c_str(), __FILE__, __LINE__, ERROR_HTTP_ERROR_RETURNED);
            throw ERROR_HTTP_ERROR_RETURNED;
        }
    } catch (int r) {
        srs_freep(http_res);
        return (ret = r);
    }

    srs_freep(http_res);
    srs_trace("http hook on_heartbeat success. "
            "client_id=%d, url=%s, request=%s, response=%s, ret=%d",
            client_id, url.c_str(), postdata.c_str(), res.c_str(), ret);

    return ret;
}

