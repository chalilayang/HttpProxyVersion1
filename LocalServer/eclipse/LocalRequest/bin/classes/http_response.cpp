#include "http_response.h"
#include "./base/Algorithm.h"
#include <boost/format.hpp>

HttpResponse::HttpResponse(const std::string& protocol_and_version, unsigned int status_code)
    : m_protocol_and_version(protocol_and_version), 
      m_status_code(status_code), 
      m_reason_str(sh_http_status_reason_str[status_code]),
      m_is_valid(true)
{

}

HttpResponse::HttpResponse(const IOBuffer& io_buf)
{
    m_header_body.assign(io_buf.data(), io_buf.size());
    m_is_valid = parse_body();
}

HttpResponse::HttpResponse(const std::string& header_lines)
{
    m_header_body = header_lines;
    m_is_valid = parse_body();
}

HttpResponse::Ptr HttpResponse::clone()
{
    HttpResponse::Ptr new_response(new HttpResponse());

    new_response->m_protocol_and_version = m_protocol_and_version;
    new_response->m_status_code = m_status_code;
    new_response->m_reason_str = m_reason_str;
    new_response->m_header_fields_map = m_header_fields_map;
    new_response->m_headers = m_headers;
    new_response->m_header_body = m_header_body;
    new_response->m_is_valid = m_is_valid;

    return new_response;
}

int64_t HttpResponse::get_content_len() const
{
    std::string len_str = get_header("Content-Length");
    if (len_str.empty())
    {
        return -1;
    }

    int64_t len;
    try
    {
        len = boost::lexical_cast<int64_t>(len_str);        
    }
    catch (boost::bad_lexical_cast&)
    {
        return -1;
    }
    return len;
}

void HttpResponse::set_content_len(int64_t content_len)
{
    std::ostringstream oss;
    oss << content_len;
    set_header("Content-Length", oss.str());
}

bool HttpResponse::parse_body()
{
    if(std::string::npos == m_header_body.find("\r\n\r\n")) return false;

    std::vector<std::string> lines;
    Splite(lines, m_header_body, "\r\n");

    //for remove regex lib and fix bug
    //BOOST_ASSERT(false == lines.empty());
    if (true == lines.empty())
        return false;

    if( std::string::npos == lines[0].find("HTTP"))
        return false;

    size_t pos_protocol =lines[0].find_first_of(' ');
    if (pos_protocol != std::string::npos) {
        m_protocol_and_version = lines[0].substr(0, pos_protocol);
    }
    else {
        return false;
    }

    size_t pos_status_beg = lines[0].find_first_not_of(' ', pos_protocol);
    if (pos_status_beg != std::string::npos) {
        size_t pos_status = lines[0].find_first_of(' ', pos_status_beg);
        if (pos_status != std::string::npos) {
            try {
                m_status_code = boost::lexical_cast<unsigned int>(
                    lines[0].substr(pos_status_beg, pos_status - pos_status_beg));
            }
            catch (boost::bad_lexical_cast&) {
                return false;
            }
        } else
        {
            return false;
        }
        
        size_t pos_reason = lines[0].find_first_not_of(' ', pos_status);
        if (pos_reason != std::string::npos) {
            m_reason_str = lines[0].substr(pos_reason);
        }
        else {
            return false;
        }
    }
//#ifdef WIN32
//#pragma warning(disable:4129)
//#endif
//    // check the protocol and version, status code and reasion string
//    boost::regex reg("^([a-zA-Z]+/[0-9\\.]+) ([0-9]+) ([\\w -]+)[\\s]*");
//#ifdef WIN32
//#pragma warning(default:4129)
//#endif
//    boost::smatch sm;
//    if (false == boost::regex_match(lines[0], sm, reg))
//    {
//        return false;
//    }
//    m_protocol_and_version = std::string(sm[1].first, sm[1].second);
//    try
//    {
//        m_status_code = boost::lexical_cast<unsigned int>(
//            std::string(sm[2].first, sm[2].second)
//            );
//    }
//    catch (boost::bad_lexical_cast&)
//    {
//        return false;
//    }
//    m_reason_str = std::string(sm[3].first, sm[3].second);

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

std::string HttpResponse::get_header(const std::string& name) const
{
    std::map<std::string, std::string>::const_iterator itr = m_header_fields_map.begin();
    for (;itr != m_header_fields_map.end(); ++itr)
    {
        if (boost::to_lower_copy(itr->first) == boost::to_lower_copy(name))
        {
            return itr->second;
        }
    }
    return "";

    std::map<std::string, std::string>::const_iterator it
        = m_header_fields_map.find(name);
    if (it == m_header_fields_map.end())
    {
        return "";
    }

    return it->second;
}

void HttpResponse::set_header(const std::string& name, const std::string& val)
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

void HttpResponse::set_header(const Header& header)
{
    set_header(header.name, header.val);
}

bool HttpResponse::has_header(const std::string& name) const
{
    std::map<std::string, std::string>::const_iterator itr = m_header_fields_map.begin();
    for (;itr != m_header_fields_map.end(); ++itr)
    {
        if (boost::to_lower_copy(itr->first) == boost::to_lower_copy(name))
        {
            return true;
        }
    }
    return false;

    std::map<std::string, std::string>::const_iterator it_map 
        = m_header_fields_map.find(name);

    if (it_map != m_header_fields_map.end())
    {
        BOOST_ASSERT(std::find_if(m_headers.begin(), m_headers.end(), HeaderEqualTo(name)) != m_headers.end());
        return true;
    }
    return false;
}

void HttpResponse::set_status_code(unsigned int status_code)
{
    m_status_code = status_code;
    m_reason_str = sh_http_status_reason_str[status_code];
}

void HttpResponse::remove_header(const std::string& name)
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

void HttpResponse::get_range(int64_t &range_beg, int64_t &range_end)
{
    std::string range_str = get_header("Content-Range");
    if (range_str.empty())
    {
        range_beg = range_end = -1;
        return;
    }

    if (boost::algorithm::starts_with(range_str, "bytes"))
    {
        // bytes <beg>-<end>/<total_length>
        std::string::size_type pos_beg, pos_bar, pos_end, pos_slash;
        pos_beg = range_str.find(' ');
        pos_bar = range_str.find('-');
        pos_slash = range_str.find('/');
        if (pos_beg == std::string::npos
         || pos_bar == std::string::npos
         || pos_slash == std::string::npos )
        {
            return;
        }
        pos_beg += 1;
        pos_end = pos_bar + 1;

        try
        {
            range_beg = boost::lexical_cast<int64_t>(range_str.substr(pos_beg, pos_bar-pos_beg));
            range_end = boost::lexical_cast<int64_t>(range_str.substr(pos_end, pos_slash-pos_end));
        }
        catch (boost::bad_lexical_cast&)
        {
            range_beg = range_end = -1;
        }
    }
    else
    {
        BOOST_ASSERT(0);    // 其他情况不支持
    }
}

bool HttpResponse::has_range() const
{
    return has_header("Content-Range");
}

void HttpResponse::remove_range()
{
    remove_header("Content-Range");
}

std::string HttpResponse::serialize_to_string()
{
    boost::format fmt_response_status_line("%1% %2% %3%\r\n");
    fmt_response_status_line 
        % m_protocol_and_version 
        % m_status_code 
        % m_reason_str;

    std::string header_lines(fmt_response_status_line.str());
    std::for_each(m_headers.begin(), m_headers.end(), HeaderAppender(header_lines));
    header_lines += "\r\n";

    return header_lines;
}

IOBuffer HttpResponse::serialize_to_buffer()
{
    return IOBuffer(serialize_to_string());
}

/*static */const char * HttpResponse::sh_http_status_reason_str[SH_HTTP_STATUS_MAX] =
{
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 

    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 

    /*100*/"Continue",
    /*101*/"Switching Protocols",
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 

    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 

    /*200*/"OK",
    /*201*/"Created",
    /*202*/"Accepted",
    /*203*/"Non-Authoritative Information",
    /*204*/"No Content",
    /*205*/"Reset Content",
    /*206*/"Partial Content",
    0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 

    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 

    /*300*/"Multiple Choices",
    /*301*/"Moved Permanently",
    /*302*/"Found",
    /*303*/"See Other",
    /*304*/"Not Modified",
    /*305*/"Use Proxy",
    /*306*/"Temporary Redirect",
    0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 

    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 

    /*400*/"Bad Request",
    /*401*/"Unauthorized", 
    /*402*/"Payment Required", 
    /*403*/"Forbidden",
    /*404*/"Not Found", 
    /*405*/"Method Not Allowed",
    /*406*/"Not Acceptable", 
    /*407*/"Proxy Authentication Required", 
    /*408*/"Request Time-out", 
    /*409*/"Conflict", 

    /*410*/"Gone",
    /*411*/"Length Required",
    /*412*/"Precondition Failed",
    /*413*/"Request Entity Too Large",
    /*414*/"Request-URI Too Large",
    /*415*/"Unsupported Media Type",
    /*416*/"Requested range not satisfiable",
    /*417*/"Expectation Failed",
    0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 

    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    /*500*/"Internal Server Error",
    /*501*/"Not Implemented",
    /*502*/"Bad Gateway",
    /*503*/"Service Unavailable",
    /*504*/"Gateway Time-out",
    /*505*/"HTTP Version not supported"
};
