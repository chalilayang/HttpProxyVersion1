/***
 *  日志系统
 *  Log具备不同模块以不同的日志级别向不同的输出流进行输出的功能。
 *
 *  ---------------------------------------------------------------------
 *  1. Log为单例
 *  2. 日志输出以模块为单位，一个模块为一个Logger，一个Logger有多个输出流(LogStream)
 *  3. 需要注册模块或为模块手工添加LogStream才能输出
 *  4. 目前提供三种输出流分别向console, file, DebugView(Windows)输出，将来可扩展
 *  5. 可设置总日志级别和各个模块(Logger)的级别
 *  6. 不同模块(Logger)可以拥有不同的输出流(LogStream)集合
 */

#ifndef _LOGGER_H_
#define _LOGGER_H_

#include "../base/common.h"

#ifdef _LOG_USE_IOS
#include "../base/ioservice_pool.h"
#endif // _LOG_USE_IOS

#ifdef ANDROID
#include <android/log.h>
#endif

class LogStream;

class Logger;

class FileLogStream;

class Log
{
public:
    enum LogLevel
    {
        LL_DISABLE = 0,
        LL_ERROR,       // 严重错误，程序紊乱或已经崩溃了
        LL_WARN,        // 一般错误，需检查设备或网络状况
        LL_EVENT,       // 正常事件
        LL_INFO,        // 详细事件
        LL_DEBUG,       // 调试信息
    };

    static Log& GetInstance()
    {
        static Log log;
        return log;
    }

    static std::string FormatTime();

    static std::string FormatLevel(int level);

    static bool LevelValid(int level);

public:
    ~Log();

    void Start();

    void Stop();

    bool LoadConfig(const char *cfg_file);

    void RegLogger(const std::string& name);

    void RegLogger(const std::string& name, int stream_type);

    Logger * GetLogger(const std::string& name);

    void SetLevel(int level);

    void SetLevel(const std::string &name, int level);

#ifdef _LOG_USE_IOS
    static boost::asio::io_service & IOS() { return ms_ios.get_ios(); }
#endif // _LOG_USE_IOS

private:
    Log();
	void OnSetLevel(int level);
#ifdef _LOG_USE_IOS
    static IOServicePool ms_ios;
#endif // _LOG_USE_IOS

private:
    std::map<std::string, Logger *> m_loggers;
    LogLevel m_root_level;
};

class Logger
{
public:
    friend class Log;

public:
    void AddStream(int stream_type);

    void AddStream(LogStream * log_stream);

    void Write(int level,const char *fmt, ...);

	void WriteWin(int level, const std::string &fun,const std::tchar_t *fmt, ...);

    void LightWrite(int level, const char *fmt, ...);

    void MemoryWrite(const char* func_name, int lineno, const char* buf, int buf_len);

    void SetLevel(const std::string& name, int level);

    int Level(const std::string& name);

    void SetLevel(int level);

    int Level() { return m_logger_level; }

    LogStream * GetLogStream(const std::string& name);

	std::string GetLogPath();

private:
    Logger(const std::string& name);
	
    ~Logger();

private:
    std::map<std::string, LogStream *> m_pstreams;
    std::string m_name;
    Log::LogLevel m_logger_level;
	FileLogStream *m_file_log;
};

class LogStream
{
public:
    friend class Logger;
    friend class LogStreamFactory;

    // stream type
    static const int con  = 0x01;   // console
    static const int file = 0x02;   // file
    static const int dbgv = 0x04;   // debugview

protected:
    LogStream(const std::string& id, int level = Log::LL_DEBUG) 
        : m_level(static_cast<Log::LogLevel>(level)), m_id(id) {}

    virtual ~LogStream() {}

    virtual void Write(std::string str) = 0;

    std::string Id() const { return m_id; }

    void Level(int level) { m_level = static_cast<Log::LogLevel>(level); }

    int Level() const { return m_level; }

    bool Canlog(int level) 
    {
        return level <= m_level; 
    }
private:
    Log::LogLevel m_level;
    std::string m_id;
};

class ConsoleLogStream : public LogStream
{
public:
    friend class LogStreamFactory;

private:
    ConsoleLogStream(const std::string& id);

    virtual ~ConsoleLogStream();

    virtual void Write(std::string str);
private:
    static int ms_incre_id;
};

class FileLogStream : public LogStream
{
public:
    friend class LogStreamFactory;

private:
    class FileFactory
    {
    public:
        FileFactory();
        ~FileFactory();

        FILE * getfile(const std::string &file_path);
        FILE * getCurrentFile(int logsize);
        //bool copyFile(const char * fileform,const char * fileto);
		std::string& GetFilePath(){return m_file_path;}
        //void SaveLog();
    private:
        std::map<std::string, FILE *> m_files;
        typedef struct FileInfo
        {
            std::string file_path;
            int file_size;
            FileInfo(std::string filepath,int filesize)
            {
                file_path = filepath;
                file_size = filesize;
            }
        }FileInfo;
        std::vector<FileInfo> m_filevec;
        int m_current_pos;
		std::string m_file_path;
    };
    class FileFactoryOld
    {
    public:
        FileFactoryOld();
        ~FileFactoryOld();

        FILE * getfile(const std::string &file_path);
		std::string& GetFilePath(){return m_file_path;}
    private:
        std::map<std::string, FILE *> m_files;
		std::string m_file_path;
    };
//#ifdef FILEFACTORYOLD
    static FileFactoryOld ms_file_factory_old;
//#else
    static FileFactory ms_file_factory;
//#endif
public:
	std::string& GetFilePath();
private:
    FileLogStream(const std::string& id, const std::string& path, bool is_single_file);

    virtual ~FileLogStream();

    virtual void Write(std::string str);

private:
    static int ms_incre_id;

    FILE *m_fp;

    bool m_is_single_file;
};

class DebugviewLogStream : public LogStream
{
public:
    friend class LogStreamFactory;
private:
    DebugviewLogStream(const std::string& id);

    virtual ~DebugviewLogStream();

    virtual void Write(std::string str);

private:
    static int ms_incre_id;
};

class LogStreamFactory
{
public:
    static LogStreamFactory& GetInstance()
    {
        static LogStreamFactory lsf;
        return lsf;
    }

    static LogStream * CreateLogStream(int stream_type);

    static ConsoleLogStream * CreateConsoleLogStream();

    static FileLogStream * CreateFileLogStream(const std::string &file_path = "");

    static DebugviewLogStream* CreateDgbViewLogStream();

public:
    // destruct all the log streams
    ~LogStreamFactory();

    void SetDefaultStream(int stream_type);

    void SetDefaultStream(LogStream * log_stream);

    LogStream * DefaultStream() const { return m_default_stream; }

    void SetDefaultFileLogStreamPath(const std::string& path) { m_default_file_stream_path = path; }

    std::string DefaultFileLogStreamPath() const { return m_default_file_stream_path; }

    void SetAsSingleFile();

private:
    // do nothing
    LogStreamFactory();

    LogStreamFactory(const LogStreamFactory&);
    const LogStreamFactory& operator=(const LogStreamFactory&);

private:
    std::map<std::string, LogStream *> m_log_streams;
    LogStream *m_default_stream;
    std::string m_default_file_stream_path;

    bool m_is_single_file;
};

inline Log& GLog()
{
    return Log::GetInstance();
}

inline LogStreamFactory& GLogStreamFactory()
{
    return LogStreamFactory::GetInstance();
}

extern std::string null_str;
extern const std::string const_null_str;

#endif