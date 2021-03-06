﻿#include "http_request.h"
#include "./base/algorithm.h"
#include <sstream>
#include <boost/format.hpp>

HttpRequest::HttpRequestPtr 
HttpRequest::create_from_url(const std::string& url, const std::string& refer_url,
                           int64_t range_beg, int64_t range_end)
{
    HttpRequest::HttpRequestPtr prequest;

    std::string protocol, host, port, path;
    boost::tie(protocol, host, port, path) = HttpRequest::ParseUrl(url);

    if (protocol.empty() || host.empty() || path.empty())
    {
        return prequest;
    }

    prequest.reset(new HttpRequest);

    prequest->m_method = "GET";
    prequest->m_protocol_and_version = "HTTP/1.1";
    prequest->m_path = path;

    prequest->set_header("Accept", "*/*");
    prequest->set_header("Accept-Language", "zh-CN");
    if (false == refer_url.empty())
    {
        prequest->set_header("Referer", refer_url);
    }
    prequest->set_header("User-Agent", "Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.1; SV1; .NET CLR 2.0.50727; .NET CLR 3.0.04506.648; .NET CLR 3.5.21022; .NET CLR 3.0.4506.2152; .NET CLR 3.5.30729)");
    prequest->set_host(port.empty() ? host : (host+":"+port));
    prequest->set_header("Connection", "Keep-Alive");
    prequest->set_range(range_beg, range_end);

    prequest->m_is_valid = true;

    return prequest;
}

boost::tuple<std::string, std::string, std::string, std::string>
HttpRequest::ParseUrl(const std::string & url)
{
    //for remove regex lib
    size_t pos_protocol = url.find("://");
    std::string protocol("http");
    if (pos_protocol != std::string::npos)
    {
        protocol = url.substr(0, pos_protocol);
    }

    size_t pos_host_beg = (pos_protocol == std::string::npos ? 0 : pos_protocol +3);
    size_t pos_path_beg = -1;
    std::string host_str("");
    std::string port_str("80");
    size_t pos_host = url.find_first_of(':', pos_host_beg);
    size_t pos_slash = url.find_first_of('/', pos_host_beg);
    if (pos_host != std::string::npos && pos_host < pos_slash)
    {
        host_str = url.substr(pos_host_beg, pos_host - pos_host_beg);
        size_t pos_port_beg = pos_host + 1;
        size_t pos_port = url.find_first_of('/', pos_port_beg);
        if (pos_port != std::string ::npos)
        {
            port_str = url.substr(pos_port_beg, pos_port - pos_port_beg);
            pos_path_beg = pos_port;
        }
    }
    else {
        pos_host = url.find_first_of('/', pos_host_beg);
        if (pos_host != std::string::npos)
        {
            host_str = url.substr(pos_host_beg, pos_host - pos_host_beg);
            pos_path_beg = pos_host;
        }
    }

    std::string path_str("");
    if (pos_path_beg > 0)
        path_str = url.substr(pos_path_beg);

    //boost::regex reg("^(([A-Za-z]+)://)?([^:/]+)(:([0-9]+))?(.*)");
    //boost::smatch sm;

    //if (false == boost::regex_match(url, sm, reg))
    //{
    //    return boost::make_tuple("", "", "", "");
    //}

    //std::string protocol(sm[2].matched ? std::string(sm[2].first, sm[2].second) : "http");
    //std::string host_str(sm[3].first, sm[3].second);
    //std::string port_str(sm[5].matched ? std::string(sm[5].first, sm[5].second) : "");
    //std::string path_str(sm[6].matched ? std::string(sm[6].first, sm[6].second) : "/");
    if (path_str.empty()) path_str = "/";

    return boost::make_tuple(protocol, host_str, port_str, path_str);
}

HttpRequest::HttpRequest()
{
    m_is_valid = false;
}

HttpRequest::HttpRequest(const IOBuffer& io_buf)
{
	m_body.assign(io_buf.data(), io_buf.size());

	m_is_valid = parse_body();
}

HttpRequest::HttpRequest(const std::string& request_lines)
{
	m_body = request_lines;

	m_is_valid = parse_body();
}

HttpRequest::Ptr HttpRequest::clone()
{
    HttpRequestPtr new_request(new HttpRequest);

    new_request->m_method = m_method;
    new_request->m_path = m_path;
    new_request->m_protocol_and_version = m_protocol_and_version;
    new_request->m_header_fields_map = m_header_fields_map;
    new_request->m_headers = m_headers;
    new_request->m_body = m_body;
    new_request->m_url = m_url;
    new_request->m_is_valid = m_is_valid;

    return new_request;
}

std::string HttpRequest::get_header(const std::string& name) const
{
    std::map<std::string, std::string>::const_iterator it
        = m_header_fields_map.find(name);
    if (it == m_header_fields_map.end())
    {
        return "";
    }

    return it->second;
}

void HttpRequest::set_header(const std::string& name, const std::string& val)
{
    std::map<std::string, std::string>::iterator it_map 
        = m_header_fields_map.find(name);

    if (it_map != m_header_fields_map.end())
    {
        HeaderFields::iterator it_list 
            = std::find_if(m_headers.begin(), m_headers.end(), HeaderEqualTo(name));
        
        BOOST_ASSERT(it_list != m_headers.end());

        it_map->second = val;
        it_list->val = val;

        return;
    }

    m_header_fields_map[name] = val;
    m_headers.push_back(Header(name, val));
}

void HttpRequest::remove_header(const std::string& name)
{
    std::map<std::string, std::string>::iterator it_map 
        = m_header_fields_map.find(name);

    if (it_map != m_header_fields_map.end())
    {
        HeaderFields::iterator it_list 
            = std::find_if(m_headers.begin(), m_headers.end(), HeaderEqualTo(name));

        BOOST_ASSERT(it_list != m_headers.end());

        m_header_fields_map.erase(it_map);
        m_headers.erase(it_list);
    }
}

void HttpRequest::set_header(const Header& header)
{
    set_header(header.name, header.val);
}

std::string HttpRequest::serialize_to_string()
{
    boost::format fmt_request_path_line("%1% %2% %3%\r\n");
    fmt_request_path_line
        % m_method
        % m_path
        % m_protocol_and_version;

    std::string header_lines(fmt_request_path_line.str());
    std::for_each(m_headers.begin(), m_headers.end(), HeaderAppender(header_lines));
    header_lines += "\r\n";

    return header_lines;
}

void HttpRequest::reset()
{
    m_is_valid = false;

    m_method.clear();
    m_url.clear();
    m_protocol_and_version.clear();
    m_headers.clear();
    m_body.clear();
}

bool HttpRequest::parse_body()
{
	if(std::string::npos == m_body.find("\r\n\r\n")) return false;

	std::vector<std::string> lines;
	Splite(lines, m_body, "\r\n");

	BOOST_ASSERT(false == lines.empty());
/*
#ifdef WIN32
#pragma warning(disable:4129)
#endif
	// check the method, path, protocol and version
	boost::regex reg("^([a-zA-Z]+) (/[^ ]*) ([a-zA-Z]+/[0-9\\.]+)");
#ifdef WIN32
#pragma warning(default:4129)
#endif
	boost::smatch sm;
	if (false == boost::regex_match(lines[0], sm, reg))
	{
		return false;
    }
    m_method = std::string(sm[1].first, sm[1].second);
    m_path = std::string(sm[2].first, sm[2].second);
    m_protocol_and_version = std::string(sm[3].first, sm[3].second);
    */
    std::vector<std::string> sm;
    Splite(sm, lines[0], " ");
    if (sm.size() != 3)
    {
        return false;
    }

    m_method = sm[0];
    m_path = sm[1];
    m_protocol_and_version = sm[2];

	// header fields
	for(int i = 1; i != lines.size(); ++i)
	{
		std::string line = lines[i];
		boost::algorithm::trim(line);
		if( line.empty() ) continue;

		size_t colon_pos = line.find_first_of(":");
		if( colon_pos == std::string::npos ) continue;

		std::string key = boost::algorithm::trim_copy(line.substr(0, colon_pos));
		std::string val = boost::algorithm::trim_copy(line.substr(colon_pos+1));

		m_headers.push_back(Header(key, val));

		// 暂不解析pragma
		//if( boost::algorithm::to_lower_copy(key) != "pragma" )
		{
			m_header_fields_map[key] = val;
		}
	}

	return true;
}

void HttpRequest::get_host_port(std::string &host, unsigned int &port) const
{
    std::string host_str = get_header("Host");
    if (host_str.empty())
    {
        return;
    }

    std::string port_str = "80";    // default port
    std::string::size_type pos = host_str.find(":");
    if (pos != std::string::npos)
    {
        port_str = host_str.substr(pos+1);
    }
    host = host_str.substr(0, pos);

    try
    {
        port = boost::lexical_cast<unsigned int>(port_str);
    }
    catch (boost::bad_lexical_cast&)
    {
        port = 80;
    }
}

void HttpRequest::set_host_port(const std::string& host, unsigned int port)
{
    std::ostringstream oss;
    oss << host;

    if (port != 80)
    {
        oss << ":" << port;
    }

    set_header("Host", oss.str());
}

void HttpRequest::set_proxy(const std::string& host, unsigned int port)
{
    if(m_path.find("http://") != 0)
    {
        m_path = "http://" + get_header("Host") + m_path;
    }
    remove_header("Connection");
    set_header("Proxy-Connection", "Keep-Alive");
    m_proxy_host = host;
    m_proxy_port = port;
}

void HttpRequest::get_conn_host_port(std::string& host, unsigned int& port) const
{
    if (m_proxy_host.empty())
    {
        get_host_port(host, port);
    }
    else
    {
        host = m_proxy_host;
        port = m_proxy_port;
    }
}

void HttpRequest::set_host_port_str(const std::string& host, const std::string &port)
{
    std::ostringstream oss;
    oss << host;
    if (port != "80" && false == port.empty())
    {
        oss << ":" << port;
    }

    set_header("Host", oss.str());
}

void HttpRequest::set_host(const std::string& host_str)
{
    if (false == host_str.empty())
    {
        set_header("Host", host_str);
    }
}

std::string HttpRequest::get_url() const
{
    std::string path, host;

    path = get_path();
    host = get_header("Host");
    if (path.empty() || host.empty())
    {
        return "";
    }

    return "Http://" + host + path;
}

bool HttpRequest::get_range(int64_t& range_beg, int64_t& range_end) const
{
    std::string range_str = get_header("Range");
    std::string::size_type pos = range_str.find("bytes=");

    if (pos == 0)   // must start with "bytes="
    {
        range_str = range_str.substr(strlen("bytes="));
        if (range_str.find(",") != std::string::npos) // For now, we don't support range set
        {
            return false;
        }
        pos = range_str.find("-");
        try
        {
            range_beg = boost::lexical_cast<int64_t>(range_str.substr(0, pos));
			if (pos + 1 < range_str.size())
				range_end = boost::lexical_cast<int64_t>(range_str.substr(pos+1));
        }
        catch (boost::bad_lexical_cast&)
        {
            return false;
        }
        return true;
    }

    return false;
}

void HttpRequest::set_range(int64_t range_beg, int64_t range_end)
{
    std::ostringstream oss;
    oss << "bytes=";

    if (range_beg > -1)
    {
        if (range_end > -1)
        {
            if (range_beg <= range_end)
            {
                oss << range_beg << "-" << range_end;
            }
            else
            {
                // invalid
                return;
            }
        }
        else
        {
            oss << range_beg << "-" ;
        }
    }
    else
    {
        if (range_end > 0)
        {
            oss << "-" << range_end;
        }
        else
        {
            // invalid
            return;
        }
    }

    set_header("Range", oss.str());
}

