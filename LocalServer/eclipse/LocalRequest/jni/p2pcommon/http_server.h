#ifndef _HTTPSERVER_H_
#define _HTTPSERVER_H_

#include "./base/common.h"
#include "./base/ioservice_pool.h"

using boost::asio::ip::tcp;

struct HttpServerHandler
{
    virtual ~HttpServerHandler() {}

    virtual void on_accept(const boost::system::error_code& ec, boost::shared_ptr<tcp::socket> p_socket) = 0;
};

class HttpServer 
    : private boost::noncopyable
    , public boost::enable_shared_from_this<HttpServer>
{
public:
    static boost::shared_ptr<HttpServer> create(boost::asio::io_service& ios, 
        const tcp::endpoint& listen_ep,unsigned int port, 
		boost::shared_ptr<HttpServerHandler> phandler);

public:
    bool start();

    void stop();

    void accept_one();

    bool is_running() const { return m_is_running; }
	unsigned int get_port() 
	{ 
		m_port = m_listen_endpoint.port();
		return m_port;	
	}

private:
    HttpServer(boost::asio::io_service& ios, const tcp::endpoint& listen_ep, unsigned int port, boost::shared_ptr<HttpServerHandler> phandler);

    void handle_accept(const boost::system::error_code& ec, boost::shared_ptr<tcp::socket> p_socket);

private:
    boost::asio::io_service& m_ios;
    tcp::endpoint   m_listen_endpoint;
	  unsigned int  m_port;
    tcp::acceptor   m_acceptor;

    boost::weak_ptr<HttpServerHandler> m_server_handler;

    volatile bool m_is_running;
};

#endif 
