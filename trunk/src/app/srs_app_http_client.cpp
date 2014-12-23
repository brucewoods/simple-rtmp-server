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

#include <srs_app_http_client.hpp>

#ifdef SRS_AUTO_HTTP_PARSER

#include <arpa/inet.h>

using namespace std;

#include <srs_app_st.hpp>
#include <srs_app_http.hpp>
#include <srs_kernel_error.hpp>
#include <srs_kernel_log.hpp>
#include <srs_app_st_socket.hpp>
#include <srs_kernel_utility.hpp>
#include <srs_app_utility.hpp>

// when error, http client sleep for a while and retry.
#define SRS_HTTP_CLIENT_SLEEP_US (int64_t)(3*1000*1000LL)

// TODO: use config file to configure this param
#define SRS_HTTP_DEFAULT_RETRY 2;

SrsHttpClient::SrsHttpClient()
{
    send_timeout = recv_timeout = ST_UTIME_NO_TIMEOUT;
    connected = false;
    stfd = NULL;
    parser = NULL;
}

SrsHttpClient::~SrsHttpClient()
{
    disconnect();
    srs_freep(parser);
}

int SrsHttpClient::post(SrsHttpUri* uri, string req, string& res)
{
    res = "";
    
    int ret = ERROR_SUCCESS;
    
    if (!parser) {
        parser = new SrsHttpParser();
        
        if ((ret = parser->initialize(HTTP_RESPONSE)) != ERROR_SUCCESS) {
            srs_error("initialize parser failed. ret=%d", ret);
            return ret;
        }
    }

    int retry = SRS_HTTP_DEFAULT_RETRY;
    while (retry--) {
        if ((ret = connect(uri)) == ERROR_SUCCESS) {
            break;
        }
    }
    if (ret != ERROR_SUCCESS) {
        srs_warn("http connect server failed. ret=%d", ret);
        return ret;
    }

    // send POST request to uri
    // POST %s HTTP/1.1\r\nHost: %s\r\nContent-Length: %d\r\n\r\n%s
    std::stringstream ss;
    ss << "POST " << uri->get_path() << " "
        << "HTTP/1.1" << __SRS_CRLF
        << "Host: " << uri->get_host() << __SRS_CRLF
        << "Connection: Keep-Alive" << __SRS_CRLF
        << "Content-Length: " << std::dec << req.length() << __SRS_CRLF
        << "User-Agent: " << RTMP_SIG_SRS_NAME << RTMP_SIG_SRS_VERSION << __SRS_CRLF
        << "Content-Type: application/x-www-form-urlencoded; charset=UTF-8" << __SRS_CRLF
        << __SRS_CRLF
        << req;
    
    SrsStSocket skt(stfd);
    skt.set_send_timeout(send_timeout);
    skt.set_recv_timeout(recv_timeout);
    
    std::string data = ss.str();
    retry = SRS_HTTP_DEFAULT_RETRY;
    while (retry--) {
        if ((ret = skt.write((void*)data.c_str(), data.length(), NULL)) == ERROR_SUCCESS) {
            break;
        }
    }
    if (ret != ERROR_SUCCESS) {
        // disconnect when error.
        disconnect();
        
        srs_error("write http post failed. ret=%d", ret);
        return ret;
    }
    
    SrsHttpMessage* msg = NULL;
    retry = SRS_HTTP_DEFAULT_RETRY;
    while (retry--) {
        if ((ret = parser->parse_message(&skt, &msg)) == ERROR_SUCCESS) {
            break;
        }
    }
    if (ret != ERROR_SUCCESS) {
        srs_error("parse http post response failed. ret=%d", ret);
        return ret;
    }

    srs_assert(msg);
    srs_assert(msg->is_complete());
    
    // get response body.
    if (msg->body_size() > 0) {
        res = msg->body();
    }
    srs_info("parse http post response success.");
    
    return ret;
}

void SrsHttpClient::disconnect()
{
    connected = false;
    
    srs_close_stfd(stfd);
}

int SrsHttpClient::connect(SrsHttpUri* uri)
{
    int ret = ERROR_SUCCESS;
    
    if (connected) {
        return ret;
    }
    
    disconnect();
    
    std::string server = uri->get_host();
    int port = uri->get_port();
    
    // open socket.
    int64_t timeout = SRS_HTTP_CLIENT_SLEEP_US;
    if ((ret = srs_socket_connect(server, port, timeout, &stfd)) != ERROR_SUCCESS) {
        srs_warn("http client failed, server=%s, port=%d, timeout=%"PRId64", ret=%d",
            server.c_str(), port, timeout, ret);
        return ret;
    }
    srs_info("connect to server success. http url=%s, server=%s, port=%d", 
        uri->get_url(), uri->get_host(), uri->get_port());
    
    connected = true;
    
    return ret;
}

int SrsHttpClient::set_recv_timeout(int64_t timeout_us) {
    recv_timeout = timeout_us;
}

int SrsHttpClient::set_send_timeout(int64_t timeout_us) {
    send_timeout = timeout_us;
}

#endif

