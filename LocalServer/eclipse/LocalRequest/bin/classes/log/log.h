//  log.h

/***
 *  Log 控制开关
 *  __LOG_CLOSE 关闭所有log
 *  __LOG_CLOSE_CONSOLE 关闭所有consolelog
 *  
 *  report bugs to ...
 */

#ifndef _LOG_H_
#define _LOG_H_

#include "logger.h"

/* -------------------------------------------------------------------- */
//  log initialization and setting

#ifndef __LOG_CLOSE
#define LOG_START() GLog().Start()
#define LOG_STOP() GLog().Stop()
#else
#define LOG_START()
#define LOG_STOP()
#endif

#ifndef __LOG_CLOSE
#define LOG_INIT_AS_FILE(log_file_path, cfg_if_log) do \
{ \
    GLogStreamFactory().SetDefaultFileLogStreamPath(log_file_path); \
    if(cfg_if_log) GLogStreamFactory().SetAsSingleFile();\
    GLogStreamFactory().SetDefaultStream(LogStream::file); \
    GLog().RegLogger("_F"); \
    GLog().RegLogger("_C", LogStream::con); \
} while (false)
#else
#define LOG_INIT_AS_FILE(log_file_path, cfg_if_log)
#endif

#ifndef __LOG_CLOSE
#define LOG_INIT_AS_CONSOLE() do \
{ \
    GLogStreamFactory().SetDefaultStream(LogStream::con); \
    GLog().RegLogger("_C", LogStream::con); \
} while (false)
#else
#define LOG_INIT_AS_CONSOLE()
#endif

#ifndef __LOG_CLOSE
#define LOG_SET_LEVEL(lv) GLog().SetLevel(lv)
#else
#define LOG_SET_LEVEL(lv)
#endif

#ifndef __LOG_CLOSE
#define LOG_SET_LOGGER_LEVEL(logger, lv) GLog().GetLogger(logger)->SetLevel(lv)
#else
#define LOG_SET_LOGGER_LEVEL(logger, lv)
#endif

#ifndef __LOG_CLOSE
#define LOG_SET_FILELOG_LEVEL(lv) GLog().GetLogger("_F")->SetLevel(lv)
#else
#define LOG_SET_FILELOG_LEVEL(lv)
#endif


/* -------------------------------------------------------------------- */
//  logger register

#ifndef __LOG_CLOSE
#define LOG_REG(logger, stream_type) GLog().RegLogger(logger, stream_type)
#else
#define LOG_REG(logger, stream_type)
#endif

#ifndef __LOG_CLOSE
#define LOG_DEFAULT_REG(logger) GLog().RegLogger(logger)
#else
#define LOG_DEFAULT_REG(logger)
#endif

#ifndef __LOG_CLOSE
#define LOG_FILE_REG(logger, file_path) do \
{ \
    FileLogStream * file_log_s = LogStreamFactory::CreateFileLogStream(file_path); \
    Logger * plogger = GLog().GetLogger(logger); \
    plogger->AddStream(file_log_s); \
} while (false)
#else
#define LOG_FILE_REG(logger, file_path)
#endif


/* -------------------------------------------------------------------- */
//  CONSOLE_LOG

#if defined (__LOG_CLOSE) || (__LOG_CLOSE_CONSOLE)
#define CONSOLE_LOG(fmt, ...)
#else
#define CONSOLE_LOG(fmt, ...) \
    GLog().GetLogger("_C")->Write(Log::LL_ERROR, fmt, ## __VA_ARGS__)
#endif

#if defined (__LOG_CLOSE) || (__LOG_CLOSE_CONSOLE)
#define L_CON_LOG(fmt, ...)
#else
#define L_CON_LOG(fmt, ...) \
    GLog().GetLogger("_C")->LightWrite(Log::LL_ERROR, fmt, __VA_ARGS__)
#endif

#define LOG_TAG    "android_p2p"
#define LOGD(...)  __android_log_print(ANDROID_LOG_DEBUG,LOG_TAG,__VA_ARGS__) // 定义LOGD类型
#define LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__) // 定义LOGI类型
#define LOGW(...)  __android_log_print(ANDROID_LOG_WARN,LOG_TAG,__VA_ARGS__) // 定义LOGW类型
#define LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__) // 定义LOGE类型
#define LOGF(...)  __android_log_print(ANDROID_LOG_FATAL,LOG_TAG,__VA_ARGS__) // 定义LOGF类型

/* -------------------------------------------------------------------- */
//  FILE_LOG

// standard log format
#ifndef __LOG_CLOSE
#define FILE_LOG_BASE(lv, fmt, ...) \
    GLog().GetLogger("_F")->Write(lv, "%s(%d) "fmt, __FUNCTION__, __LINE__, ## __VA_ARGS__)
#else
#define FILE_LOG_BASE(lv, fmt, ...)
#endif

#define FILE_LOG(fmt, ...)          FILE_LOG_BASE(Log::LL_ERROR, fmt, __VA_ARGS__)
#define FILE_LOG_ERROR(fmt, ...)    FILE_LOG_BASE(Log::LL_ERROR, fmt, __VA_ARGS__)
#define FILE_LOG_WARN(fmt, ...)     FILE_LOG_BASE(Log::LL_WARN, fmt, __VA_ARGS__)
#define FILE_LOG_EVENT(fmt, ...)    FILE_LOG_BASE(Log::LL_EVENT, fmt, __VA_ARGS__)
#define FILE_LOG_INFO(fmt, ...)     FILE_LOG_BASE(Log::LL_INFO, fmt, __VA_ARGS__)
#define FILE_LOG_DEBUG(fmt, ...)    FILE_LOG_BASE(Log::LL_DEBUG, fmt, __VA_ARGS__)

// simple log format
#ifndef __LOG_CLOSE
#define L_FILE_LOG_BASE(lv, fmt, ...) \
    GLog().GetLogger("_F")->LightWrite(lv, fmt, ## __VA_ARGS__)
#else
#define L_FILE_LOG_BASE(lv, fmt, ...)
#endif

#define L_FILE_LOG(fmt, ...) L_FILE_LOG_BASE(Log::LL_ERROR, fmt, __VA_ARGS__)

/* -------------------------------------------------------------------- */
//  LOG

// standard log format
#ifdef WIN32
#define LOG(logger, lv, fmt, ...) do \
{\
	std::ostringstream oss;\
	oss << __FUNCTION__<< " line:"<<__LINE__;\
	GLog().GetLogger(logger)->WriteWin(lv,oss.str(),fmt,## __VA_ARGS__);\
}while (false)
#else
#define LOG(logger, lv, fmt, ...)\
GLog().GetLogger(logger)->Write(lv, "[%s line:%d] "fmt, __FUNCTION__, __LINE__,## __VA_ARGS__)
#endif

#define ERROR_LOG(logger, fmt, ...) LOG(logger, Log::LL_ERROR, fmt, ## __VA_ARGS__)
#define WARN_LOG(logger, fmt, ...)  LOG(logger, Log::LL_WARN, fmt, ## __VA_ARGS__)
#define EVENT_LOG(logger, fmt, ...) LOG(logger, Log::LL_EVENT, fmt, ## __VA_ARGS__)
#define INFO_LOG(logger, fmt, ...)  LOG(logger, Log::LL_INFO, fmt, ## __VA_ARGS__)
#define DEBUG_LOG(logger, fmt, ...) LOG(logger, Log::LL_DEBUG, fmt, ## __VA_ARGS__)

// simple log format
#ifndef __LOG_CLOSE
#define L_LOG(logger, lv, fmt, ...) \
    GLog().GetLogger(logger)->LightWrite(lv, fmt, __VA_ARGS__)
#else
#define L_LOG(logger, lv, fmt, ...)
#endif

// Memory buffer dump
#ifndef __LOG_CLOSE
#define HEX_DUMP(logger, buf, buflen) \
    GLog().GetLogger(logger)->MemoryWrite(__FUNCTION__, __LINE__, buf, buflen)
#else
#define HEX_DUMP(logger, buf, buflen)
#endif

#define LOCAL_SERVER_LOG "localserver"

#endif // _FRAMEWORK_LOG_LOG_H_