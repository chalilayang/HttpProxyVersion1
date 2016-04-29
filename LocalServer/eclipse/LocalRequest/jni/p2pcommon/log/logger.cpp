#include "logger.h"
#include "../base/algorithm.h"
#include <fstream>
#include <sys/stat.h>
#include <boost/thread.hpp>
#ifdef WIN32
#include <Winbase.h>
#else
#include <sys/time.h>
#endif

#ifdef ANDROID
#include <android/log.h>
#endif

static const char *g_console_name_str = "console";
static const char *g_dbgview_name_str = "debugview";

std::string null_str = "";
const std::string const_null_str = "";

/* -------------------------------------------------------------------- */
//  class Log

#ifdef _LOG_USE_IOS
IOServicePool Log::ms_ios(1);
#endif

Log::Log() : m_root_level(LL_DISABLE)
{
}

Log::~Log()
{
    std::map<std::string, Logger *>::iterator it = m_loggers.begin();
    for (; it != m_loggers.end(); ++it)
    {
        if (it->second)
        {
            delete it->second;
        }
    }

    m_loggers.clear();
}

void Log::Start()
{
#ifdef _LOG_USE_IOS
    ms_ios.start();
#endif // _LOG_USE_IOS
}

void Log::Stop()
{
#ifdef _LOG_USE_IOS
    ms_ios.stop();
#endif // _LOG_USE_IOS
}

bool Log::LoadConfig( const char *cfg_file )
{
    // TODO
    return false;
}

void Log::RegLogger(const std::string& name)
{
    Logger *plogger = GetLogger(name);

    assert(plogger);

    if (GLogStreamFactory().DefaultStream())
    {
        plogger->AddStream(GLogStreamFactory().DefaultStream());
    }
}

void Log::RegLogger(const std::string& name, int stream_type)
{
    Logger *plogger = GetLogger(name);

    assert(plogger);

    if (stream_type & LogStream::con)
    {
        plogger->AddStream(LogStream::con);
    }
    if (stream_type & LogStream::file)
    {
        plogger->AddStream(LogStream::file);
    }
    if (stream_type & LogStream::dbgv)
    {
        plogger->AddStream(LogStream::dbgv);
    }
}

Logger * Log::GetLogger(const std::string& name)
{
    std::map<std::string, Logger *>::iterator it;
    it = m_loggers.find(name);

    if (it != m_loggers.end())
    {
        return it->second;
    }
    else
    {
        Logger * plogger = new Logger(name);
        m_loggers[name] = plogger;

        if (m_root_level != LL_DISABLE) plogger->SetLevel(m_root_level);

        return plogger;
    }
}

std::string Log::FormatTime()
{
#ifdef WIN32
    char time_buf[1024];

    SYSTEMTIME time;
    ::GetLocalTime(&time);

    sprintf_s(time_buf, 1024,
        "%04d-%02d-%02d %02d:%02d:%02d.%03d",
        time.wYear,time.wMonth,time.wDay,
        time.wHour,time.wMinute,time.wSecond, time.wMilliseconds
        );

    return time_buf;

#elif defined (_XOPEN_VERSION) && (_XOPEN_VERSION+0 >= 500)
    char time_buf[1024];
    time_t t;
    time(&t);
    tm* p = localtime(&t);

    timeval tv;
    gettimeofday(&tv, 0); //gettimeofday does not support TZ adjust on Linux.

    sprintf(time_buf, "%4d-%02d-%02d %02d:%02d:%02d.%03d",
        p->tm_year + 1900,
        p->tm_mon + 1,
        p->tm_mday,
        p->tm_hour,
        p->tm_min,
        p->tm_sec,
        tv.tv_usec / 1000
        );

    return time_buf;
    
#else
    return "";
#endif
}

std::string Log::FormatLevel(int level)
{
    switch (level)
    {
    case Log::LL_ERROR:
        return "ERROR";
    case Log::LL_WARN:
        return "WARN";
    case Log::LL_EVENT:
        return "EVENT";
    case Log::LL_INFO:
        return "INFO";
    case Log::LL_DEBUG:
        return "DEBUG";
    default:
        return "INVALID";
    }
}

bool Log::LevelValid(int level)
{
    return level >= LL_ERROR && level <= LL_DEBUG;
}

void Log::OnSetLevel(int level)
{
    if (LevelValid(level))
    {
        m_root_level = static_cast<LogLevel>(level);

        for (std::map<std::string, Logger *>::iterator it = m_loggers.begin();
            it != m_loggers.end(); ++it)
        {
            it->second->SetLevel(level);
        }
    }
}

void Log::SetLevel(const std::string &name, int level)
{
    if (LevelValid(level))
    {
        Logger * logger = GetLogger(name);
        logger->SetLevel(level);
    }
}

void Log::SetLevel(int level)
{
#ifdef _LOG_USE_IOS
	Log::IOS().post(boost::bind(&Log::OnSetLevel,this,level));
#else
	OnSetLevel(level);
#endif // _LOG_USE_IOS
}

/* -------------------------------------------------------------------- */
//  class Logger

Logger::Logger(const std::string& name)
: m_name(name), m_logger_level(Log::LL_DISABLE),m_file_log(NULL)
{

}

Logger::~Logger()
{
    // cannot destruct the log_stream which will be destructed in LogStreamFactory
    m_pstreams.clear();
}

void Logger::AddStream(int stream_type)
{
    LogStream * plog_stream = LogStreamFactory::CreateLogStream(stream_type);

    if (plog_stream)
    {
        if (m_logger_level != Log::LL_DISABLE) plog_stream->Level(m_logger_level);

        AddStream(plog_stream);
    }
}

void Logger::AddStream(LogStream * log_stream)
{
    assert(log_stream);

    if (log_stream)
    {
        m_pstreams.insert(std::make_pair(log_stream->Id(), log_stream));
    }
}

void Logger::WriteWin(int level, const std::string &fun,const std::tchar_t *fmt, ... )
{
    if (m_pstreams.empty())
    {
        return;
    }
#ifdef WIN32
	char log_str[4096];
	memset(log_str,0,sizeof(log_str));

	va_list args;  
	va_start(args, fmt);  
	int len = _vscwprintf(fmt, args)+1;
	std::wstring text;
	text.resize(len);
	vswprintf_s((wchar_t *)text.data(), len, fmt, args);  
	va_end(args); 
	text.resize(len-1);
	std::string msg_str = w2b(text);

    sprintf_s(log_str, 4096, "[%s] <%s> [%s] <%d> [%s] %s", 
        Log::FormatTime().c_str(), 
        Log::FormatLevel(level).c_str(),
        m_name.c_str(),boost::this_thread::get_id(),
		fun.c_str(),
        msg_str.c_str());

    for (std::map<std::string, LogStream *>::iterator it = m_pstreams.begin();
        it != m_pstreams.end(); ++it)
    {
        LogStream *pstream = it->second;
        if (pstream->Canlog(level))
        {
#ifdef _LOG_USE_IOS
            std::string str_log_str(log_str);
            Log::IOS().post(boost::bind(&LogStream::Write, pstream, str_log_str));
#else
           pstream->Write(log_str);
#endif // _LOG_USE_IOS
        }
    }
#endif
}

void Logger::Write(int level,const char *fmt, ...)
{
	if (m_pstreams.empty())
	{
		return;
	}

	char log_str[4096];
	memset(log_str,0,sizeof(log_str));
	char msg_str[2048];
	memset(msg_str,0,sizeof(msg_str));
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(msg_str, sizeof(msg_str), fmt, ap);
	va_end(ap);
	std::string thread_id = boost::lexical_cast<std::string>(boost::this_thread::get_id());

	sprintf(log_str, "[%s] <%s> [%s] <%s> %s",
		Log::FormatTime().c_str(), 
		Log::FormatLevel(level).c_str(),
		m_name.c_str(),thread_id.c_str(),
		msg_str);

	for (std::map<std::string, LogStream *>::iterator it = m_pstreams.begin();
		it != m_pstreams.end(); ++it)
	{
		LogStream *pstream = it->second;
		if (pstream->Canlog(level))
		{
#ifdef _LOG_USE_IOS
			std::string str_log_str(log_str);
			Log::IOS().post(boost::bind(&LogStream::Write, pstream, str_log_str));
#else
			pstream->Write(log_str);
#endif // _LOG_USE_IOS
		}
	}
}

void Logger::LightWrite(int level, const char *fmt, ...)
{
    if (m_pstreams.empty())
    {
        return;
    }

    char  msg_str[2048] ;
    memset(msg_str,0,sizeof(msg_str));
    va_list ap;
    va_start(ap, fmt);

    vsnprintf(msg_str, sizeof(msg_str), fmt, ap);

    va_end(ap);

    for (std::map<std::string, LogStream *>::iterator it = m_pstreams.begin();
        it != m_pstreams.end(); ++it)
    {
        LogStream *pstream = it->second;
        if (pstream->Canlog(level))
        {
#ifdef _LOG_USE_IOS
            std::string str_msg_str(msg_str);
            Log::IOS().post(boost::bind(&LogStream::Write, pstream, str_msg_str));
#else
            pstream->Write(msg_str);
#endif // _LOG_USE_IOS
        }
    }
}

void Logger::MemoryWrite(const char* func_name, int lineno, const char* buf, int buf_len)
{
    if (!buf || buf_len <= 0) return;

    std::size_t logbuf_size = buf_len*3+2 + 2048;
    char* logbuf = static_cast<char*>(malloc(logbuf_size));
    if (!logbuf) return;
    char* pbuf = logbuf;

#ifdef WIN32
    pbuf += sprintf_s(pbuf, logbuf_size, "[%s] <HexDump> [%s] %s(%d):\n", 
        Log::FormatTime().c_str(), 
        m_name.c_str(),
        func_name,
        lineno);
    for (int i = 0; i != buf_len; ++i)
    {
        unsigned char c = (unsigned char)buf[i];
        sprintf_s(pbuf, 3, "%2x", c);
        pbuf += 2;
        if ((i+1) % 16 == 0)
            sprintf_s(pbuf, 2, "\n");
        else
            sprintf_s(pbuf, 2, " ");
        pbuf += 1;
    }
    sprintf_s(pbuf, 2, "\n");
#else
    pbuf += sprintf(pbuf, "[%s] <HexDump> [%s] %s(%d):\n", 
        Log::FormatTime().c_str(), 
        m_name.c_str(),
        func_name,
        lineno);
    for (int i = 0; i != buf_len; ++i)
    {
        unsigned char c = (unsigned char)buf[i];
        sprintf(pbuf, "%2x", c);
        pbuf += 2;
        if ((i+1) % 16 == 0)
            sprintf(pbuf, "\n");
        else
            sprintf(pbuf, " ");
        pbuf += 1;
    }
    sprintf(pbuf, "\n");
#endif

    for (std::map<std::string, LogStream *>::iterator it = m_pstreams.begin();
        it != m_pstreams.end(); ++it)
    {
        LogStream *pstream = it->second;
#ifdef _LOG_USE_IOS
        std::string str_logbuf(logbuf);
        Log::IOS().post(boost::bind(&LogStream::Write, pstream, str_logbuf));
#else
        pstream->Write(logbuf);
#endif // _LOG_USE_IOS
    }

    free(logbuf);
}

void Logger::SetLevel(int level)
{
    if (Log::LevelValid(level))
    {
        m_logger_level = static_cast<Log::LogLevel>(level);

        for (std::map<std::string, LogStream *>::iterator it = m_pstreams.begin();
            it != m_pstreams.end(); ++it)
        {
            LogStream *pstream = it->second;
            pstream->Level(level);
        }
    }
}

void Logger::SetLevel(const std::string& name, int level)
{
    if (Log::LevelValid(level))
    {
        LogStream * pstream = GetLogStream(name);
        if (pstream)
        {
            pstream->Level(level);
        }
    }
}

int Logger::Level(const std::string& name)
{
    LogStream * pstream = GetLogStream(name);
    if (pstream)
    {
        return pstream->Level();
    }

    return Log::LL_DISABLE;
}

std::string Logger::GetLogPath()
{
	std::map<std::string, LogStream *>::iterator it = m_pstreams.begin();
	std::string fname = it->first, fpath, fex,old_path, new_path;

	size_t dash_pos = fname.rfind("_");
	old_path=fname.substr(0,dash_pos);

	size_t dot_pos = old_path.rfind(".");
	if (dot_pos != std::string::npos)
	{
		fex = old_path.substr(dot_pos);
		fpath = old_path.substr(0,dot_pos);
	}
	new_path=fpath + "_1" + fex;
	return new_path;
}

LogStream * Logger::GetLogStream(const std::string &name)
{
    std::map<std::string, LogStream *>::iterator it = m_pstreams.find(name);
    if (it != m_pstreams.end())
    {
        return it->second;
    }
    return 0;
}

/* -------------------------------------------------------------------- */
//  LogStream

static std::string IncreStreamName(const std::string &name, int &incre_id)
{
    std::ostringstream oss;
    oss << name << "_" << incre_id;
    incre_id++;
    return oss.str();
}

// consoleLogStream

int ConsoleLogStream::ms_incre_id = 0;

ConsoleLogStream::ConsoleLogStream(const std::string& id) 
    : LogStream(id)
{

}

ConsoleLogStream::~ConsoleLogStream()
{

}

void ConsoleLogStream::Write(std::string str)
{
#ifdef ANDROID
    __android_log_print(ANDROID_LOG_VERBOSE, "Android_p2p", "%s", str.c_str());
#else
    fprintf(stderr, "%s", str.c_str());
#endif
}


// FileLogStream

int FileLogStream::ms_incre_id = 0;


//#ifdef FILEFACTORYOLD
FileLogStream::FileFactoryOld FileLogStream::ms_file_factory_old;
//#else
FileLogStream::FileFactory FileLogStream::ms_file_factory;
//#endif

FileLogStream::FileLogStream(const std::string& id, const std::string& path, bool is_single_file) 
    : LogStream(id), m_is_single_file(is_single_file)
{
    if (is_single_file)
    {
        m_fp = ms_file_factory_old.getfile(path);
    }
    else
    {
        m_fp = ms_file_factory.getfile(path);
    }
}

FileLogStream::~FileLogStream()
{
}

void FileLogStream::Write(std::string str)
{
    if (!m_is_single_file)
    {
        m_fp = ms_file_factory.getCurrentFile(str.size());
    }
    if (m_fp)
    {
        fprintf(m_fp, "%s", str.c_str());
        fflush(m_fp);
    }
}

std::string& FileLogStream::GetFilePath()
{
	if (m_fp)
	{
		fflush(m_fp);
		return ms_file_factory.GetFilePath();
	}
    return null_str;
}

FileLogStream::FileFactory::FileFactory()
{
    m_current_pos = 0;
}

FileLogStream::FileFactory::~FileFactory()
{
    for (std::map<std::string, FILE *>::iterator it = m_files.begin();
        it != m_files.end(); ++it)
    {
        if (it->second)
        {
            fclose(it->second);
        }
    }
    if (!m_files.empty())
    {
        m_filevec.clear();
    }
}


FileLogStream::FileFactoryOld::FileFactoryOld()
{
}

FileLogStream::FileFactoryOld::~FileFactoryOld()
{
    for (std::map<std::string, FILE *>::iterator it = m_files.begin();
        it != m_files.end(); ++it)
    {
        if (it->second)
        {
            fclose(it->second);
        }
    }
}


namespace {
std::string TimestampForFile()
{
#ifdef WIN32
    char time_buf[1024];

    SYSTEMTIME time;
    ::GetLocalTime(&time);

    sprintf_s(time_buf, 1024,
        "%04d%02d%02d-%02d%02d%02d-%03d",
        time.wYear,time.wMonth,time.wDay,
        time.wHour,time.wMinute,time.wSecond, time.wMilliseconds
        );

    return time_buf;

#elif defined (_XOPEN_VERSION) && (_XOPEN_VERSION+0 >= 500)
    char time_buf[1024];
    time_t t;
    time(&t);
    tm* p = localtime(&t);

    timeval tv;
    gettimeofday(&tv, 0); //gettimeofday does not support TZ adjust on Linux.

    sprintf(time_buf, "%4d%02d%02d-%02d%02d%02d-%03d",
        p->tm_year + 1900,
        p->tm_mon + 1,
        p->tm_mday,
        p->tm_hour,
        p->tm_min,
        p->tm_sec,
        tv.tv_usec / 1000
        );

    return time_buf;

#else
    return "";
#endif
}
}

FILE * FileLogStream::FileFactoryOld::getfile(const std::string &file_path)
{
    std::map<std::string, FILE *>::iterator it = m_files.find(file_path);

    if (it != m_files.end())
    {
        return it->second;
    }
    else
    {
        FILE* fp_old = fopen(file_path.c_str(), "r");
        if (fp_old)
        {
            // 日志文件已存在，备份
            fclose(fp_old);
            std::ostringstream oss;
            oss << file_path << ".bak_" << TimestampForFile() << ".log";
            ::rename(file_path.c_str(), oss.str().c_str());
        }
        FILE *fp = fopen(file_path.c_str(), "at+");
        m_files.insert(std::make_pair(file_path, fp));
        return fp;
    }
}
FILE * FileLogStream::FileFactory::getfile(const std::string &file_path)
{
    std::string str = file_path, fpath, fex, new_path;
    size_t dot_pos = str.rfind(".");
    if (dot_pos != std::string::npos)
    {
        fex = str.substr(dot_pos);
        fpath = str.substr(0,dot_pos);
    }
    m_filevec.push_back(FileInfo(fpath + "_1" + fex,0));
    m_filevec.push_back(FileInfo(fpath + "_2" + fex,0));

    int sizev = m_filevec.size();
    for (int i = 0; i < sizev ;i++ )
    {
        FILE *fp = fopen(m_filevec[i].file_path.c_str(), "at+");
		if(fp!=NULL)
		{
			fseek(fp,0,SEEK_END);
			m_filevec[i].file_size=ftell(fp);
			fseek(fp,0,SEEK_SET);
			m_files.insert(std::make_pair(m_filevec[i].file_path, fp));
		}
		else
		{
			return NULL;
		}
    }
    return  getCurrentFile(0);
}
/*
bool FileLogStream::FileFactory::copyFile(const char * fileform,const char * fileto)
{
    char  tempbuf[256];
    FILE  *sfp,*dfp;     
    if (fileform == fileto)
    {
        return false;
    }
    fopen_s(&sfp,fileform,"rb");
    if (NULL == sfp)
    {
        int err = GetLastError();
        return false;
    }
    fopen_s(&dfp,fileto,"wb+"); 
    if (NULL == dfp)
    {
        int err = GetLastError();
        return false;
    }
    while(!feof(sfp))
    {
        fread(tempbuf,1,1,sfp);
        fwrite(tempbuf,1,1,dfp);
    }
    fclose(sfp);   
    fclose(dfp);  
    return true;
}

void FileLogStream::FileFactory::SaveLog()
{
    std::map<std::string, FILE *>::iterator it = m_files.begin();
    for (;it != m_files.end();++it)
    {
        std::string str = it->first, fpath, fname;
        size_t dot_pos = str.rfind("\\");
        if (dot_pos != std::string::npos)
        {
            fname = str.substr(dot_pos);
            fpath = str.substr(0,dot_pos);
        }
        std::string log_path = fpath + "\\log" + fname;
        FILE * old = it->second;
        fclose(it->second);   
        copyFile(it->first.c_str(),log_path.c_str());
        m_files[it->first] = fopen(it->first.c_str(),"wt+");

    }
}
*/
FILE * FileLogStream::FileFactory::getCurrentFile(int logsize)
{
    int sizev = m_filevec.size();
    int pre_pos = m_current_pos;

    if(m_filevec[m_current_pos].file_size> 1*1024*1024)
    {  
        m_current_pos = (m_current_pos + 1)% sizev;
       // printf("st_size > 1mb,%s next=%d \n", m_filevec[m_current_pos],m_current_pos);
    }

    m_file_path = m_filevec[m_current_pos].file_path;
    std::map<std::string, FILE *>::iterator it = m_files.find(m_file_path);

    if (it != m_files.end())
    {
        if (m_current_pos != pre_pos)
        { 
            if(m_filevec[m_current_pos].file_size  > 1*1024*1024)
            {
                fclose(it->second);
                FILE *fp = fopen(it->first.c_str(), "at+");
                m_files[m_file_path] = fp;
                m_filevec[m_current_pos].file_size = 0;
               // printf("st_size > 1mb,%s next=%d \n", it->first.c_str(),m_current_pos);
            }
        }

        m_filevec[m_current_pos].file_size += logsize;
        return it->second;
    }
    else
    {
        //assert(0);
        FILE *fp = fopen(m_file_path.c_str(), "at+");
		if(fp!=NULL)
		{
        m_files.insert(std::make_pair(m_file_path, fp));
        m_filevec[m_current_pos].file_size += logsize;
		 return fp;
		}
		else
		{
			return NULL;
		}
    }

}

// DebugviewLogStream

int DebugviewLogStream::ms_incre_id = 0;

DebugviewLogStream::DebugviewLogStream(const std::string& id) 
    : LogStream(id)
{

}

DebugviewLogStream::~DebugviewLogStream()
{

}

void DebugviewLogStream::Write(std::string str)
{
#ifdef WIN32
    ::OutputDebugStringA(str.c_str());
#endif
}

/* -------------------------------------------------------------------- */
//  LogStreamFactory

LogStream * 
LogStreamFactory::CreateLogStream(int stream_type)
{
    LogStream * plog_stream;

    switch (stream_type)
    {
    case LogStream::con :
        plog_stream = LogStreamFactory::CreateConsoleLogStream();
        break;
    case LogStream::file :
        plog_stream = LogStreamFactory::CreateFileLogStream();
        break;
    case LogStream::dbgv :
        plog_stream = LogStreamFactory::CreateDgbViewLogStream();
        break;
    default:
        plog_stream = 0;
    }

    return plog_stream;
}

ConsoleLogStream * 
LogStreamFactory::CreateConsoleLogStream()
{
    std::string con_name = IncreStreamName(g_console_name_str, ConsoleLogStream::ms_incre_id );

    std::map<std::string, LogStream *>::iterator it;
    it = GetInstance().m_log_streams.find(con_name);

    if (it != GetInstance().m_log_streams.end())
    {
        return dynamic_cast<ConsoleLogStream * >(it->second);
    }
    else
    {
        ConsoleLogStream * p = new ConsoleLogStream(con_name);
        GetInstance().m_log_streams.insert(std::make_pair(p->Id(), p));
        return p;
    }
}

FileLogStream * 
LogStreamFactory::CreateFileLogStream(const std::string &file_path)
{
    std::string valid_file_path
        = file_path.empty() ? GetInstance().DefaultFileLogStreamPath() : file_path;

    std::string f_name = IncreStreamName(valid_file_path, FileLogStream::ms_incre_id);

    std::map<std::string, LogStream *>::iterator it;
    it = GetInstance().m_log_streams.find(f_name);

    if (it != GetInstance().m_log_streams.end())
    {
        return dynamic_cast<FileLogStream * >(it->second);
    }
    else
    {
        FileLogStream *p = new FileLogStream(f_name, valid_file_path, GetInstance().m_is_single_file);
        GetInstance().m_log_streams.insert(std::make_pair(p->Id(), p));
        return p;
    }
}

DebugviewLogStream* 
LogStreamFactory::CreateDgbViewLogStream()
{
    std::string dbg_name = IncreStreamName(g_dbgview_name_str, DebugviewLogStream::ms_incre_id );

    std::map<std::string, LogStream *>::iterator it;
    it = GetInstance().m_log_streams.find(dbg_name);

    if (it != GetInstance().m_log_streams.end())
    {
        return dynamic_cast<DebugviewLogStream * >(it->second);
    }
    else
    {
        DebugviewLogStream *p = new DebugviewLogStream(dbg_name);
        GetInstance().m_log_streams.insert(std::make_pair(p->Id(), p));
        return p;
    }
}

LogStreamFactory::LogStreamFactory()
: m_default_file_stream_path("./log.log")
{
    m_default_stream = 0;
    m_is_single_file = false;
}

LogStreamFactory::~LogStreamFactory()
{
    std::map<std::string, LogStream *>::iterator it;
    for (it = m_log_streams.begin(); it != m_log_streams.end(); ++it)
    {
        LogStream * plog_stream = it->second;
        delete plog_stream;
    }
    m_log_streams.clear();
}

void LogStreamFactory::SetDefaultStream(int stream_type)
{
    LogStream * plog_stream = CreateLogStream(stream_type);

    if (plog_stream)
    {
        SetDefaultStream(plog_stream);
    }
}

void LogStreamFactory::SetDefaultStream(LogStream * log_stream)
{
    if (log_stream)
    {
        m_default_stream = log_stream;
    }
}

void LogStreamFactory::SetAsSingleFile()
{
    m_is_single_file = true;
}
