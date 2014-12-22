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

#ifndef SRS_APP_TB_LOG_HPP
#define SRS_APP_TB_LOG_HPP

#include <srs_core.hpp>

#include <srs_app_st.hpp>
#include <srs_kernel_log.hpp>
#include <srs_app_reload.hpp>
#include "srs_app_log.hpp"

#include <string.h>

#include <string>
#include <map>

// the max size of a line of log.
const int TB_LOG_MAX_SIZE = 4096;

// the tail append to each log.
const char TB_LOG_TAIL = '\n';

// reserved for the end of log data, it must be strlen(LOG_TAIL)
const int TB_LOG_TAIL_SIZE = 1;

//log file
const std::string TB_LOG_FILE = "logs/tb_live.log";
const std::string TB_WF_LOG_FILE = "logs/tb_live.log.wf";


const std::string TB_LOG_COMMON_ITEM = "product=tieba subsys=live module=srs";
const std::string LOGTYPE_HOOK = "callback";
const std::string LOGTYPE_CREATE_STREAM = "create_stream";
const std::string LOGTYPE_STREAM_STABILITY = "stream_stability";
const std::string LOGTYPE_GLOBAL_STAT = "global_stat";


class SrsIdAlloc
{
	private:
	public:
		static int64_t generate_log_id();
		static int64_t generate_conn_id();
};

/**
 * we use memory/disk cache and donot flush when write log.
 * it's ok to use it without config, which will log to console, and default trace level.
 * when you want to use different level, override this classs, set the protected _level.
 */

class SrsRequest;

class SrsTbLog : public ITbLog
{
	// for utest to override
	protected:
		// defined in SrsLogLevel.
		int _level;
	private:
		char* log_data;
		// log to file if specified srs_log_file
		int fd;
		// exception log
		int wf_fd;
		// whether log to file tank
		bool log_to_file_tank;
	public:
		SrsTbLog();
		virtual ~SrsTbLog();
	public:
		virtual int initialize();
		virtual void debug(const char* fmt, ...);
		virtual void notice(const char* fmt, ...);
		virtual void warn(const char* fmt, ...);
		virtual void error(const char* fmt, ...);
		virtual void fatal(const char* fmt, ...);
		virtual void conn_log(int log_level, std::string log_type, SrsRequest* req, const char* fmt, ...);
		virtual void global_log(int log_level, const char* fmt, ...);
	private:
		virtual bool generate_header(const char* level_name, int* header_size);
		virtual void write_log(char* str_log, int size, int level);
		virtual void open_log_file();
		virtual void open_wf_log_file();
};

#endif

