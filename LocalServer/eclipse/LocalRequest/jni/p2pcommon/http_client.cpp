#include "http_client.h"
#include <iostream>

	
HttpClientPtr HttpClientFactory(boost::asio::io_service& ios, ClientHandlerPtr phandler, 
	HttpRequest::HttpRequestPtr prequest,bool is_asyn )
{
	HttpClientPtr HttpClientPointer;
	if(is_asyn)
	{
		HttpClientPointer = Asyn_HttpClient::create(ios, phandler, prequest);
	}
	else
	{
		HttpClientPointer = Syn_HttpClient::create(ios, phandler, prequest);
	}
	return HttpClientPointer;
}

Asyn_HttpClientPtr
	Asyn_HttpClient::create(boost::asio::io_service& ios, ClientHandlerPtr phandler, 
	HttpRequest::HttpRequestPtr prequest)
{
	return Asyn_HttpClientPtr(new Asyn_HttpClient(ios, phandler, prequest));
}

Asyn_HttpClient::Asyn_HttpClient(boost::asio::io_service& ios, 
	ClientHandlerPtr phandler,
	HttpRequest::HttpRequestPtr prequest)
	: m_socket(ios), m_resolver(ios), m_client_handler(phandler), 
	m_is_closed(false), m_is_chunk(false)
{
	set_request(prequest);
}

bool Asyn_HttpClient::set_request(HttpRequest::HttpRequestPtr prequest)
{
	if (prequest)
	{
		m_request.reset();
		m_request = prequest;
		m_request->get_conn_host_port(m_target_host, m_target_port);
		return true;
	}
	return false;
}

void Asyn_HttpClient::connect()
{
	if (m_is_closed) return;

	boost::system::error_code err;
	boost::asio::ip::address addr = boost::asio::ip::address::from_string(m_target_host, err);
	if (!err)
	{
		boost::asio::ip::tcp::endpoint ep(addr, m_target_port);
		m_endpoint = ep;
		m_socket.async_connect(
			ep,
			boost::bind(
			&Asyn_HttpClient::handle_connect,
			shared_from_this(),
			boost::asio::placeholders::error,
			tcp::resolver::iterator()
			)
			);
	}
	else
	{
		tcp::resolver::query query(m_target_host,  boost::lexical_cast<std::string>(m_target_port));
		m_resolver.async_resolve(
			query,
			boost::bind(
			&Asyn_HttpClient::handle_resolve,
			shared_from_this(),
			boost::asio::placeholders::error,
			boost::asio::placeholders::iterator
			)
			);
	}
}

void Asyn_HttpClient::close()
{
	boost::system::error_code ignore_err;

	m_resolver.cancel();
	m_socket.close(ignore_err);

	//m_request.reset();
	//m_response.reset();

	m_is_closed = true;
}

void Asyn_HttpClient::handle_resolve(const boost::system::error_code& ec, 
	tcp::resolver::iterator ep_it)
{
	if (m_is_closed) return;

	if (!ec)
	{
		try
		{
			m_endpoint = ep_it->endpoint();
			m_socket.async_connect(
				*ep_it,
				boost::bind(
				&Asyn_HttpClient::handle_connect,
				shared_from_this(),
				boost::asio::placeholders::error,
				++ep_it
				)
				);
		}
		catch (...)
		{
			 //WARN_LOG("fw", "async_connect error=%s\n",ec.message().c_str());
			 boost::system::error_code my_error(1,boost::system::get_generic_category());
			 if (false == m_client_handler.expired())
			 {
				 m_client_handler.lock()->on_resolve(my_error);
			 }
			 return;
		}
	}
	else
	{
       //WARN_LOG("fw", "Resolve Error\n");
	}

	if (false == m_client_handler.expired())
	{
		m_client_handler.lock()->on_resolve(ec);
	}
}

void Asyn_HttpClient::handle_connect(const boost::system::error_code& ec, 
	tcp::resolver::iterator ep_it)
{
	if (m_is_closed) return;

	if (!ec && false == m_client_handler.expired())
	{
		m_client_handler.lock()->on_connect(ec);
	}
	else if (ep_it != tcp::resolver::iterator())
	{
		boost::system::error_code ignore_err;
		m_socket.close(ignore_err);
		m_endpoint = ep_it->endpoint();
		m_socket.async_connect(
			*ep_it,
			boost::bind(
			&Asyn_HttpClient::handle_connect, 
			shared_from_this(),
			boost::asio::placeholders::error, 
			++ep_it
			)
			);
	}
	else
	{
		if (false == m_client_handler.expired())
		{
			m_client_handler.lock()->on_connect(ec);
		}
	}
}

void Asyn_HttpClient::request()
{
	request(m_request);
}

void Asyn_HttpClient::request(HttpRequest::HttpRequestPtr prequest)
{
	if (m_is_closed) return;

	if (prequest)
	{
		set_request(prequest);
	}

	m_request->set_body(m_request->serialize_to_string());

	boost::asio::async_write(
		m_socket,
		boost::asio::buffer(m_request->body()),
		boost::bind(
		&Asyn_HttpClient::handle_write, 
		shared_from_this(),
		boost::asio::placeholders::error,
		boost::asio::placeholders::bytes_transferred
		)
		);
}

void Asyn_HttpClient::handle_write(const boost::system::error_code& ec, size_t trans_bytes)
{
	if (m_is_closed) return;

	if (!ec)
	{
		std::string delim("\r\n\r\n");

		boost::asio::async_read_until(
			m_socket, 
			m_response_buf, 
			delim,
			boost::bind(
			&Asyn_HttpClient::handle_read_header, 
			shared_from_this(),
			boost::asio::placeholders::error,
			boost::asio::placeholders::bytes_transferred
			)
			);
	}
	else
	{

	}

	if (false == m_client_handler.expired())
	{
		m_client_handler.lock()->on_write(ec);
	}
}

void Asyn_HttpClient::handle_read_header(const boost::system::error_code& ec, size_t trans_bytes)
{
	if (m_is_closed) return;

	if (!ec)
	{
		std::istream is(&m_response_buf);
        std::string str;
        std::string header;
        while (std::getline(is, header))
        {
            str += header + '\n';
            if(header == "\r")
            {
                break;
            }
        }

		IOBuffer io_buf(str);

		m_response = HttpResponse::HttpResponsePtr(new HttpResponse(io_buf));
		if (m_response && m_response->is_valid())
		{
			m_content_len = m_response->get_content_len();
			if (m_content_len == -1)
			{
				/*
				#pragma push_macro("max")
				#undef max
				m_content_len = std::numeric_limits<uint32_t>::max();
				#pragma pop_macro("max")
				*/
				m_is_chunk = true;
                //INFO_LOG("fw", "Is Chunk\n");
			}
			m_content_off = 0;
			int64_t range_beg, range_end;
			m_response->get_range(range_beg, range_end);
			if (range_beg != -1)
			{
				m_file_off = range_beg;
			}
			else
			{
				m_file_off = 0;
			}

			if (false == m_client_handler.expired())
				m_client_handler.lock()->on_read_header(ec, m_response);
		}
		else
		{
			// response 解析失败!
			if (false == m_client_handler.expired())
				m_client_handler.lock()->on_read_header(ec, HttpResponse::HttpResponsePtr());
		}
	}
	else
	{
		if (false == m_client_handler.expired())
			m_client_handler.lock()->on_read_header(ec, HttpResponse::HttpResponsePtr());
	}
}

void Asyn_HttpClient::read_content(std::size_t len)
{
	if (m_is_closed) return;

	if (m_content_off >= m_content_len)
	{
		if (false == m_client_handler.expired())
			m_client_handler.lock()->on_down();
		return;
	}

	if (m_content_off + len > m_content_len)
	{
		len = std::size_t(m_content_len - m_content_off);
	}

	if (len <= m_response_buf.size())
	{
		IOBuffer io_buf(len);
		std::istream is(&m_response_buf);
		is.read(io_buf.data(), len);

        int64_t	file_off = m_file_off;
        m_content_off += len;
        m_file_off += len;

        boost::system::error_code succeed_code;
		if (false == m_client_handler.expired())
			m_client_handler.lock()->on_read_content(succeed_code, io_buf, file_off);
	}
	else
	{
		std::size_t need_read_len = len - m_response_buf.size();

		boost::asio::async_read(
			m_socket,
			m_response_buf,
			boost::asio::transfer_at_least(need_read_len),
			boost::bind( 
			&Asyn_HttpClient::handle_read_content, 
			shared_from_this(),
			boost::asio::placeholders::error,
			boost::asio::placeholders::bytes_transferred,
			len,
			m_file_off,
			m_content_off
			)
			);
	}
}

void Asyn_HttpClient::handle_read_content(const boost::system::error_code& ec, 
	size_t trans_bytes, size_t need_len, int64_t data_file_off, int64_t data_content_off)
{
	if (m_is_closed) return;

	if (!ec)
	{
		IOBuffer io_buf(need_len);
		std::istream is(&m_response_buf);
		is.read(io_buf.data(), need_len);

        m_content_off += need_len;
        m_file_off += need_len;

		if (false == m_client_handler.expired())
			m_client_handler.lock()->on_read_content(ec, io_buf, data_file_off);
	}
	else if (ec == boost::asio::error::eof)
	{
		if (m_response_buf.size())
		{
			IOBuffer io_buf(m_response_buf.size());
			std::istream is(&m_response_buf);
			is.read(io_buf.data(), m_response_buf.size());

            m_content_off += m_response_buf.size();
            m_file_off += m_response_buf.size();
            
			if (false == m_client_handler.expired())
				m_client_handler.lock()->on_read_content(ec, io_buf, data_file_off);
		}
		else if (m_is_chunk)
		{
			if (false == m_client_handler.expired())
				m_client_handler.lock()->on_down();
		}
		else
		{
			if (false == m_client_handler.expired())
				m_client_handler.lock()->on_read_content(ec, IOBuffer(), data_file_off);
		}
	}
	else
	{
		if (false == m_client_handler.expired())
			m_client_handler.lock()->on_read_content(ec, IOBuffer(), data_file_off);
	}
}

void Asyn_HttpClient::read_chunk()
{
	if (m_is_closed) return;

	BOOST_ASSERT(m_is_chunk && m_content_len == -1);
	if (m_content_len > 0) return;
	//m_content_len = 0;

	read_chunk_size();
}

void Asyn_HttpClient::read_chunk_size()
{
    if (m_is_closed) return;

    if (m_response_buf.size() <= 2)
    {
        std::string delim("\r\n");
        boost::asio::async_read_until(
            m_socket, 
            m_response_buf, 
            delim,
            boost::bind(
            &Asyn_HttpClient::handle_read_chunk_size, 
            shared_from_this(),
            boost::asio::placeholders::error,
            boost::asio::placeholders::bytes_transferred
            )
            );
    }
    else
    {
        boost::system::error_code ec;
        handle_read_chunk_size(ec, m_response_buf.size());
    }
}

void Asyn_HttpClient::handle_read_chunk_size(const boost::system::error_code& ec, size_t trans_bytes)
{
	if (m_is_closed) return;

    if (!ec || ec == boost::asio::error::eof)
    {
        if (m_response_buf.size() <= 2)
        {
            m_client_handler.lock()->on_down();
            //DEBUG_LOG("fw", "Chunk Error\n");
        }

        std::istream is(&m_response_buf);
        is >> std::hex >> m_last_chunk_size;
        char ch;
        while(is.get(ch) && ch != '\n');

        //DEBUG_LOG("fw", "Get Chunk Size %u\n",m_last_chunk_size);
        if (m_last_chunk_size > 0 && !ec)
        {
            read_chunk_body();
        }
        else if (false == m_client_handler.expired())
        {
            if (m_last_chunk_size == 0)
            {
                m_client_handler.lock()->on_read_chunk(ec, IOBuffer(), m_content_off);
            }
            else
            {
                m_client_handler.lock()->on_down();
                //DEBUG_LOG("fw", "Chunk Error\n");
            }
        }
    }
    else
    {
        if (false == m_client_handler.expired())
            m_client_handler.lock()->on_read_chunk(ec, IOBuffer(), m_content_off);
       //DEBUG_LOG("fw", "Error %s\n",ec.message().c_str());
    }
}

void Asyn_HttpClient::read_chunk_body()
{
    if (m_is_closed) return;
    if (m_response_buf.size() >= m_last_chunk_size + 2)
    {
        boost::system::error_code ec;
        handle_read_chunk_body(ec, m_response_buf.size());
    }
    else
    {
        std::size_t need_read_len = m_last_chunk_size - m_response_buf.size() + 2;
        //DEBUG_LOG("fw", "BufSize %u; ChunkSize %u; NeedRead %u\n",m_response_buf.size(), m_last_chunk_size, need_read_len);
        boost::asio::async_read(
            m_socket, 
            m_response_buf, 
            boost::asio::transfer_at_least(need_read_len),
            boost::bind(
            &Asyn_HttpClient::handle_read_chunk_body, 
            shared_from_this(),
            boost::asio::placeholders::error,
            boost::asio::placeholders::bytes_transferred
            )
            );
    }
}
void Asyn_HttpClient::handle_read_chunk_body(const boost::system::error_code& ec, size_t trans_bytes)
{
    if (m_is_closed) return;
    if (!ec || ec == boost::asio::error::eof)
    {
        //DEBUG_LOG("fw", "BufSize %u; ChunkSize %u; TransBytes %u\n",m_response_buf.size(), m_last_chunk_size, trans_bytes);
        if (m_response_buf.size() >= m_last_chunk_size + 2)
        {
            std::istream is(&m_response_buf);
            IOBuffer io_buf(m_last_chunk_size);
            is.read(io_buf.data(), m_last_chunk_size);
            is.get();
            is.get();

            if (false == m_client_handler.expired())
                m_client_handler.lock()->on_read_chunk(ec, io_buf, m_content_off);
            m_content_off += m_last_chunk_size;
        }
        else if (ec == boost::asio::error::eof)
        {
            if (false == m_client_handler.expired())
                m_client_handler.lock()->on_down();
            //DEBUG_LOG("fw", "End of File\n");
        }
        else
        {
            read_chunk_body();
        }
    }
    else
    {
        if (false == m_client_handler.expired())
            m_client_handler.lock()->on_read_chunk(ec, IOBuffer(), 0);
        //DEBUG_LOG("fw", "Error %s\n",ec.message().c_str());
    }
}


/************************************************************************/
/*     Syn_HttpClient                                                   */
/************************************************************************/

Syn_HttpClientPtr
	Syn_HttpClient::create(boost::asio::io_service& ios, ClientHandlerPtr phandler, 
	HttpRequest::HttpRequestPtr prequest)
{
	return Syn_HttpClientPtr(new Syn_HttpClient(ios, phandler, prequest));
}

Syn_HttpClient::Syn_HttpClient(boost::asio::io_service& ios, 
	ClientHandlerPtr phandler,
	HttpRequest::HttpRequestPtr prequest)
	: m_socket(ios), m_resolver(ios), m_client_handler(phandler), 
	m_is_closed(false), m_is_chunk(false)
{
	set_request(prequest);
}


bool Syn_HttpClient::set_request(HttpRequest::HttpRequestPtr prequest)
{
	if (prequest)
	{
		m_request.reset();
		m_request = prequest;
		m_request->get_conn_host_port(m_target_host, m_target_port);
		return true;
	}
	return false;
}

void Syn_HttpClient::connect()
{
	if (m_is_closed) return;

	//////////////////////////////////////////////////////////////////////////	

	// Get a list of endpoints corresponding to the server name.
	try{
		tcp::resolver::query query( m_target_host,boost::lexical_cast<std::string>(m_target_port));
		tcp::resolver::iterator endpoint_iterator = m_resolver.resolve(query);
		tcp::resolver::iterator end;

		// Try each endpoint until we successfully establish a connection.

		boost::system::error_code error = boost::asio::error::host_not_found;
		while (error && endpoint_iterator != end)
		{
			m_socket.close();
			m_endpoint  = endpoint_iterator->endpoint();
			m_socket.connect(*endpoint_iterator++, error);
		}
		if (error)
		{
			handle_resolve(error,endpoint_iterator);
		}
		else
		{
			handle_connect(error,endpoint_iterator);
		}
	}	
	catch (std::exception& e)
	{
		std::cout << "Exception: " << e.what() << "\n";
	}
	catch (...)
	{
		std::cout << "Exception: ...\n";
	}

}

void Syn_HttpClient::close()
{
	boost::system::error_code ignore_err;

	m_resolver.cancel();
	m_socket.close(ignore_err);

	//m_request.reset();
	//m_response.reset();

	m_is_closed = true;
}

void Syn_HttpClient::handle_resolve(const boost::system::error_code& ec, 
	tcp::resolver::iterator ep_it)
{
	if (m_is_closed) return;

	//m_endpoint = ep_it->endpoint();

	if (false == m_client_handler.expired())
	{
		m_client_handler.lock()->on_resolve(ec);
	}
	//////////////////////////////////////////////////////////////////////////
}

void Syn_HttpClient::handle_connect(const boost::system::error_code& ec, 
	tcp::resolver::iterator ep_it)
{
	if (m_is_closed) return;

	//m_endpoint = ep_it->endpoint();

	if (false == m_client_handler.expired())
	{
		m_client_handler.lock()->on_connect(ec);
	}

}

void Syn_HttpClient::request()
{
	request(m_request);
}

void Syn_HttpClient::request(HttpRequest::HttpRequestPtr prequest)
{
	if (m_is_closed) return;

	if (prequest)
	{
		set_request(prequest);
	}

	m_request->set_body(m_request->serialize_to_string());
	boost::system::error_code err;
	std::size_t trans_bytes = boost::asio::write(m_socket,boost::asio::buffer(m_request->body()),boost::asio::transfer_all(),err);//无异常

	handle_write(err,trans_bytes);
}

void Syn_HttpClient::handle_write(const boost::system::error_code& ec, size_t trans_bytes)
{
	if (m_is_closed) return;

	if (false == m_client_handler.expired())
	{
		m_client_handler.lock()->on_write(ec);
	}

	if (!ec)
	{
		std::string delim("\r\n\r\n");
		boost::system::error_code err;
		std::size_t trans_bytes = boost::asio::read_until(m_socket,m_response_buf,delim,err);//无异常

		handle_read_header(err,trans_bytes);
	}
	else
	{

	}

}

void Syn_HttpClient::handle_read_header(const boost::system::error_code& ec, size_t trans_bytes)
{
	if (m_is_closed) return;

	if (!ec)
	{
		std::istream is(&m_response_buf);
		IOBuffer io_buf(trans_bytes);
		is.read(io_buf.data(), trans_bytes);

		m_response = HttpResponse::HttpResponsePtr(new HttpResponse(io_buf));
		if (m_response && m_response->is_valid())
		{
			m_content_len = m_response->get_content_len();
			if (m_content_len == -1)
			{
				/*
				#pragma push_macro("max")
				#undef max
				m_content_len = std::numeric_limits<uint32_t>::max();
				#pragma pop_macro("max")
				*/
				m_is_chunk = true;
			}
			m_content_off = 0;
			int64_t range_beg, range_end;
			m_response->get_range(range_beg, range_end);
			if (range_beg != -1)
			{
				m_file_off = range_beg;
			}
			else
			{
				m_file_off = 0;
			}

			if (false == m_client_handler.expired())
				m_client_handler.lock()->on_read_header(ec, m_response);
		}
		else
		{
			// response 解析失败!
			if (false == m_client_handler.expired())
				m_client_handler.lock()->on_read_header(ec, HttpResponse::HttpResponsePtr());
		}
	}
	else
	{
		if (false == m_client_handler.expired())
			m_client_handler.lock()->on_read_header(ec, HttpResponse::HttpResponsePtr());
	}
}

void Syn_HttpClient::read_content(std::size_t len)
{
	if (m_is_closed) return;

	if (m_content_off >= m_content_len)
	{
		if (false == m_client_handler.expired())
			m_client_handler.lock()->on_down();
		return;
	}

	if (m_content_off + len > m_content_len)
	{
		len = std::size_t(m_content_len - m_content_off);
	}

	if (len <= m_response_buf.size())
	{
		IOBuffer io_buf(len);
		std::istream is(&m_response_buf);
		is.read(io_buf.data(), len);

		boost::system::error_code succeed_code;
		if (false == m_client_handler.expired())
			m_client_handler.lock()->on_read_content(succeed_code, io_buf, m_file_off);

		m_content_off += len;
		m_file_off += len;
	}
	else
	{
		std::size_t need_read_len = len - m_response_buf.size();		

		boost::system::error_code err;
		std::size_t trans_bytes = boost::asio::read(m_socket,m_response_buf,boost::asio::transfer_at_least(need_read_len),err);//无异常
		handle_read_content(err,trans_bytes,len,m_file_off,m_content_off);

		m_content_off += len;
		m_file_off += len;
	}
}

void Syn_HttpClient::handle_read_content(const boost::system::error_code& ec, 
	size_t trans_bytes, size_t need_len, int64_t data_file_off, int64_t data_content_off)
{
	if (m_is_closed) return;

	if (!ec)
	{
		IOBuffer io_buf(need_len);
		std::istream is(&m_response_buf);
		is.read(io_buf.data(), need_len);

		if (false == m_client_handler.expired())
			m_client_handler.lock()->on_read_content(ec, io_buf, data_file_off);
	}
	else if (ec == boost::asio::error::eof)
	{
		if (m_response_buf.size())
		{
			IOBuffer io_buf(m_response_buf.size());
			std::istream is(&m_response_buf);
			is.read(io_buf.data(), m_response_buf.size());

			if (false == m_client_handler.expired())
				m_client_handler.lock()->on_read_content(ec, io_buf, data_file_off);
		}
		else if (m_is_chunk)
		{
			if (false == m_client_handler.expired())
				m_client_handler.lock()->on_down();
		}
		else
		{
			if (false == m_client_handler.expired())
				m_client_handler.lock()->on_read_content(ec, IOBuffer(), data_file_off);
		}
	}
	else
	{
		if (false == m_client_handler.expired())
			m_client_handler.lock()->on_read_content(ec, IOBuffer(), data_file_off);
	}
}

void Syn_HttpClient::read_chunk()
{
	if (m_is_closed) return;

	BOOST_ASSERT(m_is_chunk && m_content_len == -1);
	if (m_content_len > 0) return;
	m_content_len = 0;		

	std::string delim("\r\n\r\n");
	boost::system::error_code err;
	std::size_t trans_bytes = boost::asio::read_until(m_socket,m_response_buf,delim,err);//无异常

	handle_read_chunk_size(err,trans_bytes);
}

void Syn_HttpClient::read_chunk_size()
{
	std::string delim("\r\n\r\n");
	boost::system::error_code err;
	std::size_t trans_bytes = boost::asio::read_until(m_socket,m_response_buf,delim,err);//无异常

	handle_read_chunk_size(err,trans_bytes);
}

void Syn_HttpClient::handle_read_chunk_size(const boost::system::error_code& ec, size_t trans_bytes)
{
	if (m_is_closed) return;

}
