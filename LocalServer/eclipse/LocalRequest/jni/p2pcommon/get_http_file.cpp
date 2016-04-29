#include "get_http_file.h"
#include "./log/log.h"
#include "./base/algorithm.h"


boost::shared_ptr<GetHttpFile> GetHttpFile::create(boost::asio::io_service& ios)
{
    return boost::shared_ptr<GetHttpFile>(new GetHttpFile(ios));
}

void GetHttpFile::get_data(ResultFuncType func,
    const std::string& url,
	bool is_asyn, const std::string& ref_url, 
    int64_t range_start, int64_t range_end)
{
    m_func = func;
    m_content_buf = IOBuffer();

    m_request = HttpRequest::create_from_url(url, ref_url, range_start, range_end);
    if (m_user_agent_str.empty() == false)
    {			
        m_request->set_header("User-Agent", m_user_agent_str);
    }
    if (m_cookie_str.empty() == false)
    {
        m_request->set_header("Cookie", m_cookie_str);
    }
    if (m_proxy_host.empty() == false && m_proxy_port > 0)
    {
        m_request->set_proxy(m_proxy_host, m_proxy_port);
    }
    m_is_asyn = is_asyn;

    m_http_client = HttpClientFactory(m_ios, shared_from_this(), m_request, m_is_asyn);
    m_http_client->connect();
}

tcp::endpoint GetHttpFile::get_host() const
{
	return m_http_client->get_host();
}
void GetHttpFile::close()
{
    if (m_http_client)
    {
        m_http_client->close();
        m_http_client.reset();
    }

    m_is_close = true;
}

void GetHttpFile::on_resolve(const boost::system::error_code& ec)
{
    if (m_is_close) return;

    if (ec)
    {
        m_func(IOBuffer(), ec);
    }
}

void GetHttpFile::on_connect(const boost::system::error_code& ec)
{
    if (m_is_close) return;

    if (ec)
    {
        m_func(IOBuffer(), ec);
    }
    else
    {
        m_http_client->request();
    }
}

void GetHttpFile::on_write(const boost::system::error_code& ec)
{
    if (m_is_close) return;

    if (ec)
    {
        m_func(IOBuffer(), ec);
    }
}

void GetHttpFile::on_read_header(const boost::system::error_code& ec, HttpResponse::HttpResponsePtr p_response)
{
    if (m_is_close) return;

    try {
        if (!ec && p_response)
        {
            //EVENT_LOG("fw", "[%p] Response  ok!!\n%s\n", this, p_response->SerializeToString().c_str());
            if (p_response->get_status_code() == HttpResponse::SH_HTTP_STATUS_OK
                || p_response->get_status_code() == HttpResponse::SH_HTTP_STATUS_PARTIAL_CONTENT)
            {
                m_content_len = p_response->get_content_len();
                if (m_content_len > 0 && m_content_len < 10*1024*1024) // < 10MB ?
                {
                    m_http_client->read_content(size_t(m_content_len));
                }
                else if(m_http_client->is_chunked())
                {
                    m_http_client->read_chunk();
                }
                else
                {
                    //WARN_LOG("fw", "No ContentLength But Not Chunked\n");
                    m_func(IOBuffer(), ec);
                }
            }
            else if (p_response->get_status_code() == HttpResponse::SH_HTTP_STATUS_MOVED
                     || p_response->get_status_code() ==HttpResponse::SH_HTTP_STATUS_REDIRECT)
            {
                std::string real_url = p_response->get_header("Location");
                if (real_url.empty() == false)
                {
                    std::string real_path, real_host;
                    if (real_url.find("http://") == 0)
                    {
                        size_t start_pos = strlen("http://");
                        size_t end_pos = real_url.find('/',start_pos);
                        if (end_pos != std::string::npos)
                        {
                            real_host = real_url.substr(start_pos,end_pos-start_pos);
                            real_path = real_url.substr(end_pos);
                        }
                        else
                        {
                            real_host = real_url.substr(start_pos);
                            real_path = "/";
                        }
                        m_request->set_host(real_host);
                    }
                    else
                    {
                        real_path = real_url;
                    }
                    m_request->set_path(real_path);
                    m_http_client = HttpClientFactory(m_ios, shared_from_this(), m_request, m_is_asyn);
                    m_http_client->connect();
                }
                else
                {
                    m_func(IOBuffer(), ec);
                }
            }
			else
			{
				if (m_request)
				{
					std::tstring request_url = b2w(m_request->get_url());
					EVENT_LOG("download", _T("HTPP 返回状态码错误: this=0x%08x, statuts_code=%d, request_url=%s\n"), this, \
						p_response->get_status_code(), request_url.c_str());
				}
			}
        }
        else
        {
            m_func(IOBuffer(), ec);
        }
    }
    catch (...)
    {
        return;
    }
}

void GetHttpFile::on_read_content(const boost::system::error_code& ec, const IOBuffer& io_buf, int64_t off)
{
    if (m_is_close) return;

    if (!ec)
    {
        if (off + io_buf.size() == m_content_len)
        {
            m_func(io_buf, ec);
        }
        else
        {
            m_func(io_buf, ec); // it's bad? ~~~
            //INFO_LOG("fw", "ReceivedLength != ContentLength\n");
        }
    }
    else
    {
        m_func(IOBuffer(), ec);
    }
}

void GetHttpFile::on_read_chunk(const boost::system::error_code& ec, const IOBuffer& io_buf, int64_t off)
{
    if (m_is_close) return;

    if (!ec || ec == boost::asio::error::eof)
    {
        if (io_buf.empty())
        {
            m_func(m_content_buf, ec);
            //INFO_LOG("fw", "Receive Buffer by Chunk\n");
        }
        else if(!ec)
        {
            IOBuffer tmp_buf(m_content_buf.size()+io_buf.size());
            if(m_content_buf.empty() == false)
                memcpy(tmp_buf.data(), m_content_buf.data(), m_content_buf.size());
            memcpy(tmp_buf.data()+m_content_buf.size(), io_buf.data(), io_buf.size());
            m_content_buf = tmp_buf;
            m_http_client->read_chunk();
        }
        else
        {
            m_func(IOBuffer(), ec);
        }
    }
    else
    {
        m_func(IOBuffer(), ec);
    }
}

void GetHttpFile::on_down()
{
    // will never reach this ?

    if (m_is_close) return;
    //INFO_LOG("fw", "on_down\n");
    boost::system::error_code succeed_ec;
    m_func(IOBuffer(), succeed_ec);
}
