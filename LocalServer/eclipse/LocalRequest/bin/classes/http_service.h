#ifndef _HTTP_SERVICE_H_
#define _HTTP_SERVICE_H_

#include "./base/common.h"
#include "./base/iobuffer.h"
#include "http_request.h"
#include "http_response.h"
#include <queue>

using boost::asio::ip::tcp;

struct HttpServiceHandler
{
    virtual ~HttpServiceHandler() {}

    virtual void on_service_close() = 0;
    virtual void on_service_read(const boost::system::error_code& ec, HttpRequest::HttpRequestPtr p_request, IOBuffer buf) = 0;
    virtual void on_service_read_timeout() = 0;
    virtual void on_service_write(const boost::system::error_code& ec, size_t trans_bytes) = 0;
};

class HttpService : private boost::noncopyable, public boost::enable_shared_from_this<HttpService >
{
public:
    typedef boost::shared_ptr<HttpService> Ptr;

    static HttpService::Ptr create(boost::shared_ptr<tcp::socket> psock, boost::shared_ptr<HttpServiceHandler> phandler)
    {
        if (psock)
        {
            return HttpService::Ptr (new HttpService(psock, phandler));
        }
        return HttpService::Ptr();
    }

public:
    explicit HttpService(boost::shared_ptr<tcp::socket> psock, boost::shared_ptr<HttpServiceHandler> phandler, std::size_t timeout_sec = 30);

    virtual ~HttpService();

    void start();

    void read_header();

    void stop();

    void close();

    void write(const IOBuffer& io_buf);

private:
    void handle_read(const boost::system::error_code& ec, size_t trans_bytes);

    void handle_read_timeout();

    void handle_write(const boost::system::error_code& ec, size_t trans_bytes, const IOBuffer& buf);

private:
    typedef boost::shared_ptr<boost::asio::deadline_timer> TimerPtr;

    boost::shared_ptr<tcp::socket> m_psocket;
    HttpRequest::HttpRequestPtr m_request;
    boost::asio::streambuf m_request_buf;

    std::size_t m_timeout_sec;
    TimerPtr m_read_timer;
 //   TimerPtr m_close_timer;
    boost::weak_ptr<HttpServiceHandler> m_service_handler;

    // first stop, then close it after 1 second.
    bool m_is_stopped;
    bool m_is_closed;

	std::queue<IOBuffer> qwiobuff_;
	IOBuffer       cwiobuff_;
};

#endif
