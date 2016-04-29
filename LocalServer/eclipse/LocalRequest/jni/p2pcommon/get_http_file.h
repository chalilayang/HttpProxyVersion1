#ifndef _GETHTTPFILE_H_
#define _GETHTTPFILE_H_

#include "./base/common.h"
#include "./base/iobuffer.h"
#include "http_client.h"

class GetHttpFile
    : private boost::noncopyable
    , public boost::enable_shared_from_this<GetHttpFile>
    , public ClientHandler
{
public:
    typedef boost::function<void (IOBuffer,const boost::system::error_code&)> ResultFuncType;
    typedef boost::shared_ptr<GetHttpFile> Ptr;

    static boost::shared_ptr<GetHttpFile> create(boost::asio::io_service& ios);

public:
    virtual ~GetHttpFile() { close();}

    inline void set_cookie(const std::string &cookie_str)
    {
        m_cookie_str = cookie_str;
    }
    inline void set_proxy(const std::string &host, unsigned int port)
    {
        m_proxy_host = host;
        m_proxy_port = port;
    }
    inline void set_user_agent(const std::string &user_agent_str)
    {
        m_user_agent_str = user_agent_str;
    }

    void get_data(ResultFuncType func,
        const std::string& url,
		bool is_asyn = true, const std::string& ref_url = "", 
        int64_t range_start = -1, int64_t range_end = -1);

	tcp::endpoint get_host() const;

    void close();

    virtual void on_resolve(const boost::system::error_code& ec);
    virtual void on_connect(const boost::system::error_code& ec);
    virtual void on_write(const boost::system::error_code& ec);
    virtual void on_read_header(const boost::system::error_code& ec, HttpResponse::HttpResponsePtr p_response);
    virtual void on_read_content(const boost::system::error_code& ec, const IOBuffer& io_buf, int64_t off);
    virtual void on_read_chunk(const boost::system::error_code& ec, const IOBuffer& io_buf, int64_t off);
    virtual void on_down();

private:
    GetHttpFile(boost::asio::io_service& ios)
        : m_ios(ios), m_is_asyn(false), m_is_close(false) {}

private:
    boost::shared_ptr<HttpClient> m_http_client;	
    boost::asio::io_service& m_ios;
    ResultFuncType m_func;
    int64_t m_content_len;
    IOBuffer m_content_buf;
    std::string m_proxy_host;
    unsigned int m_proxy_port;
    std::string m_cookie_str;
    std::string m_user_agent_str;
    boost::shared_ptr<HttpRequest> m_request;
    bool m_is_asyn;
    bool m_is_close;
};

#endif
