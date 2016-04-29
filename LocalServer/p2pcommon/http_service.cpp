#include "http_service.h"
#include <boost/asio.hpp>
#include "./log/log.h"
#include "./base/algorithm.h"
#include "../include/macro_define.h"

CHttpService::CHttpService(boost::shared_ptr<tcp::socket> psock, 
                         boost::shared_ptr<CHttpServiceHandler> phandler, 
                         std::size_t timeout_sec)
    : m_psocket(psock), m_service_handler(phandler), m_timeout_sec(timeout_sec)
{
 //   m_close_timer = TimerPtr(new boost::asio::deadline_timer(psock->io_service()));

    m_is_stopped = false;
    m_is_closed = false;
}

CHttpService::~CHttpService()
{

}

void CHttpService::start()
{
    read_header();
}

void CHttpService::read_header()
{
	if (m_is_closed)
	{
		return;
	}

    std::string delim("\r\n\r\n");
    boost::asio::async_read_until(
        *m_psocket, 
        m_request_buf, 
        delim,
        boost::bind(
        &CHttpService::handle_read, 
        shared_from_this(),
        boost::asio::placeholders::error,
        boost::asio::placeholders::bytes_transferred
        )
        );

    m_read_timer = TimerPtr(new boost::asio::deadline_timer(m_psocket->get_io_service()));
    m_read_timer->expires_from_now(boost::posix_time::seconds(m_timeout_sec));
    m_read_timer->async_wait(boost::bind(&CHttpService::handle_read_timeout, shared_from_this()));
}

void CHttpService::stop()
{
    if (m_is_stopped) return;

    boost::system::error_code ignore_error;
    m_psocket->lowest_layer().shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignore_error);    

    m_is_stopped = true;

//    m_close_timer->expires_from_now(boost::posix_time::seconds(1));
//	m_close_timer->async_wait(boost::bind(&CHttpService::close, shared_from_this()));
	close();
}

void CHttpService::close()
{
    boost::system::error_code ignore_error;

	m_psocket->lowest_layer().shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignore_error);    
	m_psocket->lowest_layer().close(ignore_error);
	m_psocket.reset();

	m_is_stopped = true;
    m_is_closed = true;
	/*
	if (m_close_timer)
	{
		m_close_timer->cancel(ignore_error);
		m_close_timer.reset();
	}
	*/
	if (m_request)
	{
		m_request.reset();
	}
	INFO_LOG("download", _T("CHttpService::close() localserver \n"));

    //if (false == m_service_handler.expired())
    //{
    //    m_service_handler.lock()->on_service_close();
    //}
}

void CHttpService::handle_read(const boost::system::error_code& ec, size_t trans_bytes)
{
    if (m_is_stopped || m_service_handler.expired())
    {
        return;
    }
	boost::system::error_code ignore_error;
	if (m_read_timer)
	{
		m_read_timer->cancel(ignore_error);
		m_read_timer.reset();
	}
    if (!ec)
    {
		INFO_LOG(LOCAL_SERVER_LOG, _T("成功读取到 http 请求数据! socket: 0x%08x\n"), m_psocket.get());

        if (m_request)
        {
            m_request.reset();
        }

        std::istream is(&m_request_buf);
        char tmp_buf[2048];
        std::string lines;
        while(is.getline(tmp_buf,2048))
        {
            lines += std::string(tmp_buf) + "\n";
            if (tmp_buf[0] == '\r')break;
        }
        
        IOBuffer io_buf(lines);
        m_request = HttpRequest::HttpRequestPtr(new HttpRequest(io_buf));

        if (m_request && m_request->is_valid())
        {
            if(m_request->get_method() == "POST")
            {
                size_t content_length = atoi(m_request->get_header("Content-Length").c_str());
                if (content_length > 0)
                {
                    boost::shared_array<char> b = boost::shared_array<char>(new char[content_length]);
                    size_t ct = m_request_buf.size(), rt = 0;
                    is.read(b.get(),m_request_buf.size());
                    try
                    {
                        while((rt = boost::asio::read(*m_psocket, boost::asio::buffer(b.get()+ct, content_length-ct))))
                        {
                            ct += rt;
                            if (ct == content_length)
                            {
                                break;
                            }
                        }
                        if (ct == content_length)
                        {
                            IOBuffer content_buf(b.get(), content_length);
                            m_service_handler.lock()->on_service_read(ec, m_request, content_buf);
                        }
                        else
                        {
                            m_service_handler.lock()->on_service_read(ec, m_request, IOBuffer());
                        }
                    }
                    catch(...)
                    {
                        m_service_handler.lock()->on_service_read(ec, m_request, IOBuffer());
                    }
                }
                else
                {
                    m_service_handler.lock()->on_service_read(ec, m_request, IOBuffer());
                }
            }
            else
            {
                m_service_handler.lock()->on_service_read(ec, m_request, IOBuffer());
            }
            
        }
        else
        {
            m_service_handler.lock()->on_service_read(ec, HttpRequest::HttpRequestPtr(), IOBuffer());
        }
    }
    else
    {
        m_service_handler.lock()->on_service_read(ec, HttpRequest::HttpRequestPtr(), IOBuffer());
    }
}

void CHttpService::handle_read_timeout()
{
    // "!m_read_timer" means that we had stopped the timer
    if (m_is_stopped || m_service_handler.expired() || !m_read_timer)
    {
        return;
    }

    m_service_handler.lock()->on_service_read_timeout();
}

void CHttpService::write(const IOBuffer& io_buf)
{
    if (m_is_stopped)
    {
        return;
    }

    boost::asio::async_write(
        *m_psocket,
        boost::asio::buffer(io_buf.data(), io_buf.size()),
        boost::bind(
            &CHttpService::handle_write,
            shared_from_this(),
            boost::asio::placeholders::error,
            boost::asio::placeholders::bytes_transferred,
            io_buf
        )
    );
}

void CHttpService::handle_write(const boost::system::error_code& ec, size_t trans_bytes, const IOBuffer&)
{
    if (m_is_stopped || m_service_handler.expired())
    {
        return;
    }

    m_service_handler.lock()->on_service_write(ec, trans_bytes);
}
bool CHttpService::is_queue_empty(void)
{
	return qwiobuff_.size() <= LOCAL_SERVER_BUFFER_SIZE;
}

boost::shared_ptr<tcp::socket>& CHttpService::get_socket(void)
{
	return m_psocket;
}
