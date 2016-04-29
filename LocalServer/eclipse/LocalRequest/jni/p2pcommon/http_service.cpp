#include "http_service.h"
#include <boost/asio.hpp>
#include "./log/log.h"
#include "./base/algorithm.h"
#include "../include/macro_define.h"

HttpService::HttpService(boost::shared_ptr<tcp::socket> psock, 
                         boost::shared_ptr<HttpServiceHandler> phandler, 
                         std::size_t timeout_sec)
    : m_psocket(psock), m_service_handler(phandler), m_timeout_sec(timeout_sec)
{
 //   m_close_timer = TimerPtr(new boost::asio::deadline_timer(psock->io_service()));

    m_is_closed_ = false;
}

HttpService::~HttpService()
{

}

void HttpService::start(void)
{
    read_header();
}

void HttpService::close(void)
{
	if (m_is_closed_) return;
	m_is_closed_ = true;

	boost::system::error_code ignore_error;
	m_psocket->lowest_layer().shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignore_error); 
	m_psocket->lowest_layer().close(ignore_error);
	m_psocket.reset();

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

    if (!m_service_handler.expired())
    {
        m_service_handler.lock()->on_service_close();
    }
}

void HttpService::read_header(void)
{
	if (m_is_closed_)
	{
		return;
	}

	std::string delim("\r\n\r\n");
	boost::asio::async_read_until(
		*m_psocket, 
		m_request_buf, 
		delim,
		boost::bind(
		&HttpService::handle_read, 
		shared_from_this(),
		boost::asio::placeholders::error,
		boost::asio::placeholders::bytes_transferred
		)
		);

	/*
	boost::system::error_code ignore_error;
	if (m_read_timer)
	{
		m_read_timer->cancel(ignore_error);
		m_read_timer.reset();
	}

	m_read_timer = TimerPtr(new boost::asio::deadline_timer(m_psocket->get_io_service()));
	m_read_timer->expires_from_now(boost::posix_time::seconds(m_timeout_sec));
	m_read_timer->async_wait(boost::bind(&HttpService::handle_read_timeout, shared_from_this()));*/
}

void HttpService::handle_read(const boost::system::error_code& ec, size_t trans_bytes)
{
    if (m_is_closed_ || m_service_handler.expired())
    {
		//INFO_LOG(LOCAL_SERVER_LOG, _T("HTTP 连接已关闭, 读取请求信息直接返回! socket=0x%08x\n"), m_psocket.get());
        return;
    }

	//收到的字节数为0，直接返回
	if (trans_bytes == 0)
	{
		//INFO_LOG(LOCAL_SERVER_LOG, _T("读取HTTP请求信息字节数为0, 直接返回! socket=0x%08x, error: %d, desp: %s\r\n"), m_psocket.get(), ec.value(), b2w(ec.message()).c_str());
		return;
	}

	//关闭定时器
	/*boost::system::error_code ignore_error;
	if (m_read_timer)
	{
		m_read_timer->cancel(ignore_error);
		m_read_timer.reset();
	}*/

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

		//INFO_LOG(LOCAL_SERVER_LOG, _T("http内容 : %s\n"), b2w(lines).c_str());
		        
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
				//HTTP GET方式请求
                m_service_handler.lock()->on_service_read(ec, m_request, IOBuffer());
            } 

			//读取下一次请求
			read_header();
        }
        else
        {
			INFO_LOG(LOCAL_SERVER_LOG, _T("HTTP请求的URL无效! socket: 0x%08x\n"), m_psocket.get());
            m_service_handler.lock()->on_service_read(ec, HttpRequest::HttpRequestPtr(), IOBuffer());
        }
    }
    else
    {
		INFO_LOG(LOCAL_SERVER_LOG, _T("读取 http 请求数据失败! socket: 0x%08x, error: %d, desp: %s\n"), m_psocket.get(), ec.value(), b2w(ec.message()).c_str());
		m_service_handler.lock()->on_service_read(ec, HttpRequest::HttpRequestPtr(), IOBuffer());
    }
}

void HttpService::handle_read_timeout()
{
    if (m_is_closed_ || m_service_handler.expired() || !m_read_timer)
    {
        return;
    }

	//INFO_LOG(LOCAL_SERVER_LOG, _T("读取 http 请求超时! socket: 0x%08x\n"), m_psocket.get());
    m_service_handler.lock()->on_service_read_timeout();
}

void HttpService::write(const IOBuffer& io_buf)
{
    if (m_is_closed_)
    {
		INFO_LOG(LOCAL_SERVER_LOG, _T("停止发送数据: qwiobuff_.size() == %d, len=%d, socket=0x%08x\r\n"), qwiobuff_.size(), io_buf.size(), m_psocket.get());
        return;
    }

	if (cwiobuff_.empty())
	{
		cwiobuff_ = io_buf.clone();
	}
	else
	{
		//INFO_LOG(LOCAL_SERVER_LOG, _T("发送数据: qwiobuff_.size()=%d, len=%d, socket=0x%08x\r\n"), qwiobuff_.size(), io_buf.size(), m_psocket.get());
		qwiobuff_.push(io_buf.clone());
		return;
	}

    boost::asio::async_write(
        *m_psocket,
        boost::asio::buffer(io_buf.data(), io_buf.size()),
        boost::bind(
            &HttpService::handle_write,
            shared_from_this(),
            boost::asio::placeholders::error,
            boost::asio::placeholders::bytes_transferred,
            io_buf
        )
    );

	//INFO_LOG(LOCAL_SERVER_LOG, _T("发送数据: cwiobuff_为空, qwiobuff_.size()=%d, len=%d, socket=0x%08x\r\n"), qwiobuff_.size(), io_buf.size(), m_psocket.get());
}

void HttpService::handle_write(const boost::system::error_code& ec, size_t trans_bytes, const IOBuffer& buf)
{
	//INFO_LOG(LOCAL_SERVER_LOG, _T("收到数据: qwiobuff_.size()=%d, len=%d, socket=0x%08x\r\n"), qwiobuff_.size(), trans_bytes, m_psocket.get());

    if (m_is_closed_ || m_service_handler.expired())
    {
		INFO_LOG(LOCAL_SERVER_LOG, _T("播放器收到数据直接返回, socket=0x%08x, m_is_closed_=%d, qwiobuff_.size() == %d\r\n"), m_psocket.get(), m_is_closed_, qwiobuff_.size());
        return;
    }

	if (!ec)
	{
		if (cwiobuff_.size() == 0)
		{
			INFO_LOG(LOCAL_SERVER_LOG, _T("缓冲区大小错误! cwiobuff_.size() == 0 socket: 0x%08x\n"), m_psocket.get());
		}

		if (trans_bytes > 0)
		{
			cwiobuff_.consume(trans_bytes);
		}

		if (cwiobuff_.size() == 0)
		{
			if (false == m_service_handler.expired())
			{
				m_service_handler.lock()->on_service_write(ec,trans_bytes);
			}

			if (!qwiobuff_.empty())
			{
				cwiobuff_ = qwiobuff_.front();
				qwiobuff_.pop();
				m_service_handler.lock()->on_service_request_video_data(qwiobuff_.size());
			}
		}

		if (cwiobuff_.size() > 0)
		{
			boost::asio::async_write(
				*m_psocket,
				boost::asio::buffer(cwiobuff_.data(), cwiobuff_.size()),
				boost::bind(
				&HttpService::handle_write,
				shared_from_this(),
				boost::asio::placeholders::error,
				boost::asio::placeholders::bytes_transferred,
				buf
				)
				);
		}

		//INFO_LOG(LOCAL_SERVER_LOG, _T("发送数据: qwiobuff_.size()=%d, len=%d, socket=0x%08x\r\n"), qwiobuff_.size(), cwiobuff_.size(), m_psocket.get());
	}
	else 
	{
		INFO_LOG(LOCAL_SERVER_LOG, _T("上层HTTP 连接发生异常! socket=0x%08x, %d, %s\n"), m_psocket.get(), ec.value(), b2w(ec.message()).c_str());

		m_service_handler.lock()->on_service_write(ec, trans_bytes);
	}
}

bool HttpService::is_queue_empty(void)
{
	return qwiobuff_.size() <= LOCAL_SERVER_BUFFER_SIZE;
}

boost::shared_ptr<tcp::socket>& HttpService::get_socket(void)
{
	return m_psocket;
}
