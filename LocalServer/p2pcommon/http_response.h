#ifndef _HTTPRESPONSE_H_
#define _HTTPRESPONSE_H_

#include "./base/common.h"
#include "./base/iobuffer.h"
#include "http_header.h"

class HttpResponse
	: private boost::noncopyable
{
public:
	enum StatusCode
	{
        SH_HTTP_STATUS_CONTINUE             = 100, // OK to continue with request
        SH_HTTP_STATUS_SWITCH_PROTOCOLS     = 101, // server has switched protocols in upgrade header

        SH_HTTP_STATUS_OK                   = 200, // request completed
        SH_HTTP_STATUS_CREATED              = 201, // object created, reason = new URI
        SH_HTTP_STATUS_ACCEPTED             = 202, // async completion (TBS)
        SH_HTTP_STATUS_PARTIAL              = 203, // partial completion
        SH_HTTP_STATUS_NO_CONTENT           = 204, // no info to return
        SH_HTTP_STATUS_RESET_CONTENT        = 205, // request completed, but clear form
        SH_HTTP_STATUS_PARTIAL_CONTENT      = 206, // partial GET furfilled

        SH_HTTP_STATUS_AMBIGUOUS            = 300, // server couldn't decide what to return
        SH_HTTP_STATUS_MOVED                = 301, // object permanently moved
        SH_HTTP_STATUS_REDIRECT             = 302, // object temporarily moved
        SH_HTTP_STATUS_REDIRECT_METHOD      = 303, // redirection w/ new access method
        SH_HTTP_STATUS_NOT_MODIFIED         = 304, // if-modified-since was not modified
        SH_HTTP_STATUS_USE_PROXY            = 305, // redirection to proxy, location header specifies proxy to use
        SH_HTTP_STATUS_REDIRECT_KEEP_VERB   = 307, // HTTP/1.1: keep same verb

        SH_HTTP_STATUS_BAD_REQUEST          = 400, // invalid syntax
        SH_HTTP_STATUS_DENIED               = 401, // access denied
        SH_HTTP_STATUS_PAYMENT_REQ          = 402, // payment required
        SH_HTTP_STATUS_FORBIDDEN            = 403, // request forbidden
        SH_HTTP_STATUS_NOT_FOUND            = 404, // object not found
        SH_HTTP_STATUS_BAD_METHOD           = 405, // method is not allowed
        SH_HTTP_STATUS_NONE_ACCEPTABLE      = 406, // no response acceptable to client found
        SH_HTTP_STATUS_PROXY_AUTH_REQ       = 407, // proxy authentication required
        SH_HTTP_STATUS_REQUEST_TIMEOUT      = 408, // server timed out waiting for request
        SH_HTTP_STATUS_CONFLICT             = 409, // user should resubmit with more info
        SH_HTTP_STATUS_GONE                 = 410, // the resource is no longer available
        SH_HTTP_STATUS_LENGTH_REQUIRED      = 411, // the server refused to accept request w/o a length
        SH_HTTP_STATUS_PRECOND_FAILED       = 412, // precondition given in request failed
        SH_HTTP_STATUS_REQUEST_TOO_LARGE    = 413, // request entity was too large
        SH_HTTP_STATUS_URI_TOO_LONG         = 414, // request URI too long
        SH_HTTP_STATUS_UNSUPPORTED_MEDIA    = 415, // unsupported media type
        SH_HTTP_STATUS_RETRY_WITH           = 449, // retry after doing the appropriate action.

        SH_HTTP_STATUS_SERVER_ERROR         = 500, // internal server error
        SH_HTTP_STATUS_NOT_SUPPORTED        = 501, // required not supported
        SH_HTTP_STATUS_BAD_GATEWAY          = 502, // error response received from gateway
        SH_HTTP_STATUS_SERVICE_UNAVAIL      = 503, // temporarily overloaded
        SH_HTTP_STATUS_GATEWAY_TIMEOUT      = 504, // timed out waiting for gateway
        SH_HTTP_STATUS_VERSION_NOT_SUP      = 505, // HTTP version not supported
        SH_HTTP_STATUS_MAX,
	};

    static const char *sh_http_status_reason_str[SH_HTTP_STATUS_MAX];

    typedef boost::shared_ptr<HttpResponse> HttpResponsePtr;
    typedef boost::shared_ptr<HttpResponse> Ptr;

public:
    HttpResponse() {}

    HttpResponse(const std::string& protocol_and_version, unsigned int status_code);

    explicit HttpResponse(const IOBuffer& io_buf);

    explicit HttpResponse(const std::string& header_lines);

    HttpResponsePtr clone();

    bool parse_body();

    bool is_valid() const { return m_is_valid; }

    int64_t get_content_len() const;

    void set_content_len(int64_t content_len);

    std::string get_header(const std::string& name) const;

    /*
     *  If "name" exist, change the value, if not, append it.
     */
    void set_header(const std::string& name, const std::string& val);

    void set_header(const Header& header);

    void set_status_code(unsigned int status_code);

    bool has_header(const std::string& name) const;

    void remove_header(const std::string& name);

    void get_range(int64_t &range_beg, int64_t &range_end);

    bool has_range() const;

    void remove_range();

    std::string serialize_to_string();

    IOBuffer serialize_to_buffer();

	unsigned int get_status_code() const { return m_status_code; }

    std::string get_reason_string() const { return m_reason_str; }

private:
// 	string response_header_string_;
// 	volatile bool response_modified_;
// 
// 	size_t range_begin_;
// 	size_t range_end_;
// 	size_t file_length_;

	std::string m_protocol_and_version;
    unsigned int m_status_code;
    std::string m_reason_str;
	std::map<std::string, std::string> m_header_fields_map;
	HeaderFields m_headers;
	std::string m_header_body;

    bool m_is_valid;
};

#endif
