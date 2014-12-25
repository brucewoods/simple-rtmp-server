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

#include <srs_app_tb_log.hpp>

#include <stdarg.h>
#include <sys/time.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <srs_app_config.hpp>
#include <srs_kernel_error.hpp>
#include <srs_app_utility.hpp>
#include <srs_kernel_utility.hpp>
#include <srs_protocol_rtmp.hpp>

#include <stdlib.h>

const int DEFAULT_TB_LOG_LEVEL = TbLogLevel::Notice;

using namespace std;

int64_t SrsIdAlloc::generate_log_id()
{
    timeval tv;
    if (gettimeofday(&tv, NULL) == -1) {
        return 0;
    }

    struct tm* tm;
    if ((tm = localtime(&tv.tv_sec)) == NULL) {
        return 0;
    }

    srand((unsigned)time(0));
    int random_num = rand()  % 1000;

    return (tm->tm_min * 60LL + tm->tm_sec) * 1000000LL + tv.tv_usec + random_num;
}

int64_t SrsIdAlloc::generate_conn_id()
{
    timeval tv;
    if (gettimeofday(&tv, NULL) == -1) {
        return 0;
    }

    struct tm* tm;
    if ((tm = localtime(&tv.tv_sec)) == NULL) {
        return 0;
    }

    srand((unsigned)time(0));
    int random_num = rand()  % 1000;

    return (tm->tm_min * 60LL + tm->tm_sec) * 1000000LL + tv.tv_usec + random_num;
}


SrsTbLog::SrsTbLog()
{
    _level = DEFAULT_TB_LOG_LEVEL;
    log_data = new char[TB_LOG_MAX_SIZE];

    fd = -1;
    wf_fd = -1;
    log_to_file_tank = true;
}

SrsTbLog::~SrsTbLog()
{
    srs_freep(log_data);

    if (fd > 0) {
        ::close(fd);
        fd = -1;
    }

    if (wf_fd > 0) {
        ::close(wf_fd);
        wf_fd = -1;
    }
}

int SrsTbLog::initialize()
{
    int ret = ERROR_SUCCESS;
    log_to_file_tank = true;
    _level = 0x03;

    return ret;
}

void SrsTbLog::debug(const char* fmt, ...)
{
    if (_level > TbLogLevel::Debug)
    {
        return;
    }
    int size = 0;
    if (!generate_header("DEBUG", &size)) {
        return;
    }

    va_list ap;
    va_start(ap, fmt);
    // we reserved 1 bytes for the new line.
    size += vsnprintf(log_data + size, TB_LOG_MAX_SIZE - size, fmt, ap);
    va_end(ap);

    write_log(log_data, size, TbLogLevel::Debug);
}

void SrsTbLog::notice(const char* fmt, ...)
{
    if (_level > TbLogLevel::Notice)
    {
        return;
    }
    int size = 0;
    if (!generate_header("NOTICE", &size)) {
        return;
    }

    va_list ap;
    va_start(ap, fmt);
    // we reserved 1 bytes for the new line.
    size += vsnprintf(log_data + size, TB_LOG_MAX_SIZE - size, fmt, ap);
    va_end(ap);

    write_log(log_data, size, TbLogLevel::Notice);
}

void SrsTbLog::warn(const char* fmt, ...)
{
    if (_level > TbLogLevel::Warn) {
        return;
    }

    int size = 0;
    if (!generate_header("WARNING", &size)) {
        return;
    }

    va_list ap;
    va_start(ap, fmt);
    // we reserved 1 bytes for the new line.
    size += vsnprintf(log_data + size, TB_LOG_MAX_SIZE - size, fmt, ap);
    va_end(ap);

    write_log(log_data, size, TbLogLevel::Warn);
}

void SrsTbLog::error(const char* fmt, ...)
{
    if (_level > TbLogLevel::Error) {
        return;
    }

    int size = 0;
    if (!generate_header("ERROR", &size)) {
        return;
    }

    va_list ap;
    va_start(ap, fmt);
    // we reserved 1 bytes for the new line.
    size += vsnprintf(log_data + size, TB_LOG_MAX_SIZE - size, fmt, ap);
    va_end(ap);

    write_log(log_data, size, TbLogLevel::Error);
}

void SrsTbLog::fatal(const char* fmt, ...)
{
    if (_level > TbLogLevel::Fatal) {
        return;
    }

    int size = 0;
    if (!generate_header("FATAL", &size)) {
        return;
    }

    va_list ap;
    va_start(ap, fmt);
    // we reserved 1 bytes for the new line.
    size += vsnprintf(log_data + size, TB_LOG_MAX_SIZE - size, fmt, ap);
    va_end(ap);

    write_log(log_data, size, TbLogLevel::Fatal);
}

void SrsTbLog::conn_log(int log_level, string log_type, SrsRequest* req, const char * fmt,...)
{
    char msg[TB_LOG_MAX_SIZE] = "";
    va_list ap;
    va_start(ap, fmt);
    // we reserved 1 bytes for the new line.
    int real_size = vsnprintf(msg, TB_LOG_MAX_SIZE, fmt, ap);
    string str_msg(msg, real_size);
    va_end(ap);
    stringstream ss;
    ss << "["
        << "logid=" << SrsIdAlloc::generate_log_id() << " "
        << TB_LOG_COMMON_ITEM << " "
        << "log_type=" << log_type << " "
        << "client_type=" << req->client_info->client_type << " "
        << "client_version=" << req->client_info->client_version << " " 
        << "user_role=" << req->client_info->user_role << " "
        << "net_type=" << req->client_info->net_type << " "
        << "conn_id=" << req->client_info->conn_id << " "
        << "user_id=" << req->client_info->user_id << " "
        << "group_id=" << req->client_info->group_id << " "
        << str_msg
        << "]";
    string log_body = ss.str();
    switch(log_level)
    {
        case TbLogLevel::Debug:
            {
                tb_debug(log_body.c_str());
                break;
            }
        case TbLogLevel::Notice:
            {
                tb_notice(log_body.c_str());
                break;
            }
        case TbLogLevel::Warn:
            {
                tb_warn(log_body.c_str());
                break;
            }
        case TbLogLevel::Error:
            {
                tb_error(log_body.c_str());
                break;
            }
        case TbLogLevel::Fatal:
            {
                tb_fatal(log_body.c_str());
                break;
            }
        default:
            {
                tb_warn("wrong log type!");
            }
    }
    return;    
}

void SrsTbLog::global_log(int log_level,const char * fmt,...)
{
    char msg[TB_LOG_MAX_SIZE] = "";
    va_list ap;
    va_start(ap, fmt);
    // we reserved 1 bytes for the new line.
    int real_size = vsnprintf(msg, TB_LOG_MAX_SIZE, fmt, ap);
    string str_msg(msg, real_size);
    va_end(ap);
    stringstream ss;
    ss << "["
        << "logid=" << SrsIdAlloc::generate_log_id() << " "
        << TB_LOG_COMMON_ITEM << " "
        << "log_type=" << LOGTYPE_GLOBAL_STAT << " "
        << str_msg
        << "]";
    string log_body = ss.str();
    switch(log_level)
    {
        case TbLogLevel::Debug:
            {
                tb_debug(log_body.c_str());
                break;
            }
        case TbLogLevel::Notice:
            {
                tb_notice(log_body.c_str());
                break;
            }
        case TbLogLevel::Warn:
            {
                tb_warn(log_body.c_str());
                break;
            }
        case TbLogLevel::Error:
            {
                tb_error(log_body.c_str());
                break;
            }
        case TbLogLevel::Fatal:
            {
                tb_fatal(log_body.c_str());
                break;
            }
        default:
            {
                tb_warn("wrong log type!");
            }
    }
    return;    
}

bool SrsTbLog::generate_header(const char* level_name, int* header_size)
{
    // clock time
    timeval tv;
    if (gettimeofday(&tv, NULL) == -1) {
        return false;
    }

    // to calendar time
    struct tm* tm;
    if ((tm = localtime(&tv.tv_sec)) == NULL) {
        return false;
    }

    // write log header
    int log_header_size = -1;

    log_header_size = snprintf(log_data, TB_LOG_MAX_SIZE, 
            "%s: %d-%02d-%02d %02d:%02d:%02d TB_LIVE ", 
            level_name, 1900 + tm->tm_year, 1 + tm->tm_mon, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);

    if (log_header_size == -1) {
        return false;
    }

    // write the header size.
    *header_size = srs_min(TB_LOG_MAX_SIZE - 1, log_header_size);

    return true;
}

void SrsTbLog::write_log(char *str_log, int size, int level)
{
    // ensure the tail and EOF of string
    //      LOG_TAIL_SIZE for the TAIL char.
    //      1 for the last char(0).
    size = srs_min(TB_LOG_MAX_SIZE - 1 - TB_LOG_TAIL_SIZE, size);

    // add some to the end of char.
    str_log[size++] = TB_LOG_TAIL;
    //str_log[size++] = 0;

    if (level < TbLogLevel::Warn)       //notice
    {
        if (fd < 0)
        {
            open_log_file();
        }

        if (fd > 0)
        {
            ::write(fd, str_log, size);
        }
    }
    else
    {
        if (wf_fd < 0)
        {
            open_wf_log_file();
        }

        if (wf_fd > 0)
        {
            ::write(wf_fd, str_log, size);
        }
    }
    memset(log_data, 0, TB_LOG_MAX_SIZE);
}

void SrsTbLog::open_log_file()
{
    std::string filename = TB_LOG_FILE;

    if (filename.empty()) {
        return;
    }

    fd = ::open(filename.c_str(), O_RDWR | O_APPEND);

    if(fd == -1 && errno == ENOENT) {
        fd = open(filename.c_str(), 
                O_RDWR | O_CREAT | O_TRUNC, 
                S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH
                );
    }
}

void SrsTbLog::open_wf_log_file()
{
    std::string filename = TB_WF_LOG_FILE;

    if (filename.empty()) {
        return;
    }

    wf_fd = ::open(filename.c_str(), O_RDWR | O_APPEND);

    if(wf_fd == -1 && errno == ENOENT) {
        wf_fd = open(filename.c_str(), 
                O_RDWR | O_CREAT | O_TRUNC, 
                S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH
                );
    }
}



