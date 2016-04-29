#ifndef _HTTPREQUEST_H_
#define _HTTPREQUEST_H_

#include "./base/common.h"
#include "./base/iobuffer.h"
#include "http_header.h"

class HttpRequest
{
public:
	typedef boost::shared_ptr<HttpRequest> HttpRequestPtr;
	typedef boost::shared_ptr<HttpRequest> Ptr;

	static HttpRequestPtr create_from_url(const std::string& url, 
		const std::string& refer_url = "", int64_t range_beg = -1, int64_t range_end = -1);

	// return <protocol, host_str, port_str, path_str>
	static boost::tuple<std::string, std::string, std::string, std::string> ParseUrl(const std::string& url);

public:
	HttpRequest();

	explicit HttpRequest(const IOBuffer& io_buf);

	explicit HttpRequest(const std::string& request_lines);

	HttpRequestPtr clone();

	void set_body(const std::string& body) { m_body = body; }

	bool parse_body();

	std::string& body() { return m_body; }

	bool is_valid() const { return m_is_valid; }

	std::string get_method() const { return m_method; }

	void set_method(const std::string& method) { m_method = method; }

	std::string get_path() const { return m_path; }

	void set_path(const std::string& npath) { m_path = npath; }

	std::string get_header(const std::string& name) const;

	/*
	*  If "name" exist, change the value, if not, append it.
	*/
	void set_header(const std::string& name, const std::string& val);

	void set_header(const Header& header);

	void remove_header(const std::string& name);

	void get_host_port(std::string& host, unsigned int& port) const;

	void set_host_port(const std::string& host, unsigned int port);

	void set_proxy(const std::string& host, unsigned int port);

	void get_conn_host_port(std::string& host, unsigned int& port) const;

	void set_host_port_str(const std::string& host, const std::string &port);

	/*
	*  host_str == "127.0.0.1" or "127.0.0.1:1234" or "www.baidu.com" or "www.baidu.com:8080"
	*/
	void set_host(const std::string& host_str);

	std::string get_url() const;

	bool get_range(int64_t& range_beg, int64_t& range_end) const;

	void set_range(int64_t range_beg, int64_t range_end);

	/*
	*  (TODO) support the range set
	*/
	// void GetRanges(const RangeSpecifier&) const;
	// void SetRanges(const RangeSpecifier&);
	// void set_range(const Range&);

	std::string serialize_to_string();

	/// reset to initial state.
	void reset();

private:
	std::string m_method;
	std::string m_path;
	std::string m_protocol_and_version;
	std::map<std::string, std::string> m_header_fields_map;
	HeaderFields m_headers;
	std::string m_body;

	std::string m_url;

	std::string m_proxy_host;
	unsigned int m_proxy_port;

	bool m_is_valid;
};

#endif
