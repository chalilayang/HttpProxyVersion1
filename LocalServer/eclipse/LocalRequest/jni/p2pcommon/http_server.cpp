#include "http_server.h"
#include "./log/log.h"
#include "./base/algorithm.h"
#include "../localhttpserver/local_http_server.h"

boost::shared_ptr<HttpServer>
HttpServer::create(boost::asio::io_service& ios, const tcp::endpoint& listen_ep, unsigned int port, 
                   boost::shared_ptr<HttpServerHandler> phandler)
{
    return boost::shared_ptr<HttpServer>(new HttpServer(ios, listen_ep, port, phandler));
}

HttpServer::HttpServer(boost::asio::io_service& ios,
                       const tcp::endpoint& listen_ep,unsigned int port,
                       boost::shared_ptr<HttpServerHandler> phandler)
                       : m_ios(ios),
                       m_listen_endpoint(listen_ep),m_port(port),
                       m_acceptor(m_ios),
                       m_server_handler(phandler),
                       m_is_running(false)
{

}

bool HttpServer::start()
{
	if (true == m_is_running)
	{
		// logic error
		INFO_LOG(LOCAL_SERVER_LOG, _T("m_is_running is true !\r\n"));
		return false;
	}

	boost::system::error_code ec;

	for (int i = 0 ; i < 5; ++i)
	{
		ec.clear();
		if (m_acceptor.is_open())
		{
			m_acceptor.close(ec);
		}	

		if (ec)
		{
			//open failed
			std::string errstr = ec.message();
			INFO_LOG(LOCAL_SERVER_LOG, _T("is_open() fail: %s! port=%d\r\n"), b2w(errstr).c_str(), m_port);
			continue;
		}
		boost::asio::ip::address addr = boost::asio::ip::address::from_string("127.0.0.1");
		m_listen_endpoint = tcp::endpoint(addr, m_port);
		m_acceptor.open(m_listen_endpoint.protocol(), ec);
		if (ec)
		{
			//open failed
			std::string errstr = ec.message();
			INFO_LOG(LOCAL_SERVER_LOG, _T("open port fail %s! IP: %s, port: %d\r\n"), b2w(errstr).c_str(), b2w(addr.to_string()).c_str(), m_port);
			continue;
		}
		m_acceptor.set_option(tcp::acceptor::reuse_address(false), ec);
		m_acceptor.set_option(tcp::acceptor::linger(true, 0), ec);

		if (ec)
		{
			//open failed
			std::string errstr = ec.message();
			INFO_LOG(LOCAL_SERVER_LOG, _T("set_option() fail: %s! port=%d\r\n"), b2w(errstr).c_str(), m_port);
			continue;
		}

		m_acceptor.bind(m_listen_endpoint, ec);
		if (ec)
		{
			// bind address failed			
			std::string errstr = ec.message();
			INFO_LOG(LOCAL_SERVER_LOG, _T("bind() fail: %s! port=%d\r\n"), b2w(errstr).c_str(), m_port);
			continue;
		}

		m_acceptor.listen(0x7fffffff,ec);
		if (ec)
		{
			// listen address failed			
			std::string errstr = ec.message();
			INFO_LOG(LOCAL_SERVER_LOG, _T("listen() fail: %s! port=%d\r\n"), b2w(errstr).c_str(), m_port);
			continue;
		}

		INFO_LOG(LOCAL_SERVER_LOG, _T("监听 IP: %s, 端口: %d\r\n"), b2w(addr.to_string()).c_str(), m_port);

		break;		
	}

	if (ec)
	{
		//open or bind address failed
		INFO_LOG(LOCAL_SERVER_LOG, _T("start() return false !\r\n"));
		return false;
	}

	m_is_running = true;

	//accept_one();

	return true;
}

void HttpServer::stop()
{
    if (false == m_is_running)
    {
        // logic error
        return;
    }

    boost::system::error_code ec;
    m_acceptor.cancel(ec);
    m_acceptor.close(ec);

    m_is_running = false;
}

void HttpServer::accept_one()
{
    if (false == m_is_running)
    {
        return;
    }

	boost::shared_ptr<tcp::socket> p_socket(new tcp::socket(LocalHttpServer::Inst()->get_io_service(K_THREAD_SEND)));//(new tcp::socket(m_ios));

	//INFO_LOG(LOCAL_SERVER_LOG, _T("监听 socket=0x%08x, use_count=%d\r\n"), p_socket.get(), p_socket.use_count());
    
    m_acceptor.async_accept(
        *p_socket,
        boost::bind(
            &HttpServer::handle_accept,
            shared_from_this(),
            boost::asio::placeholders::error,
            p_socket
        )
    );
}

void HttpServer::handle_accept(const boost::system::error_code& ec, boost::shared_ptr<tcp::socket> p_socket)
{
	if (false == m_is_running)
	{
		return;
	}

	try
	{
		if (!ec)
		{
			if (!m_server_handler.expired())
			{
				//INFO_LOG(LOCAL_SERVER_LOG, _T("收到http 请求! socket=0x%08x, use_count=%d\r\n"), p_socket.get(), p_socket.use_count());
				m_server_handler.lock()->on_accept(ec, p_socket);
			}
		}
		else if (ec == boost::asio::error::operation_aborted)
		{
			INFO_LOG(LOCAL_SERVER_LOG, _T("收到http 请求失败 ec == boost::asio::error::operation_aborted, socket = 0x%08x, error: %d, desp: %s\n"), p_socket.get(), ec.value(), b2w(ec.message()).c_str());
			return;
		}

		//接收下一个http请求
		accept_one();
	}
	catch (...)
	{
		INFO_LOG(LOCAL_SERVER_LOG, _T("收到http 请求发生了异常! socket = 0x%08x\n"), p_socket.get());
	}
}
