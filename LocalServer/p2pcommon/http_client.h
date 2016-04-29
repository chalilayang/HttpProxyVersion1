#ifndef _HTTPCLIENT_H_
#define _HTTPCLIENT_H_

#include "./base/common.h"
#include "./base/iobuffer.h"
#include "http_request.h"
#include "http_response.h"


using boost::asio::ip::tcp;

struct ClientHandler;
struct HttpClient;
class Syn_HttpClient;
class Asyn_HttpClient;

typedef boost::weak_ptr<ClientHandler> ClientHandlerPtr;
typedef boost::shared_ptr<HttpClient> HttpClientPtr;
typedef boost::shared_ptr<Syn_HttpClient> Syn_HttpClientPtr;
typedef boost::shared_ptr<Asyn_HttpClient> Asyn_HttpClientPtr;
struct ClientHandler
{
	virtual ~ClientHandler() {}

	virtual void on_resolve(const boost::system::error_code& ec) = 0;

	virtual void on_connect(const boost::system::error_code& ec) = 0;

	virtual void on_write(const boost::system::error_code& ec) = 0;

	virtual void on_read_header(const boost::system::error_code& ec, HttpResponse::HttpResponsePtr p_response) = 0;

	virtual void on_read_content(const boost::system::error_code& ec, const IOBuffer& io_buf, int64_t off) = 0;

    virtual void on_read_chunk(const boost::system::error_code& ec, const IOBuffer& io_buf, int64_t off) = 0;

	virtual void on_down() = 0;
};



struct  HttpClient : boost::noncopyable
{

	virtual ~HttpClient(){}
	virtual bool set_request(HttpRequest::HttpRequestPtr prequest) = 0;

	virtual void connect() = 0;

	virtual void request() = 0;

	virtual void request(HttpRequest::HttpRequestPtr prequest) = 0;

	virtual void read_chunk() = 0;

	virtual void read_content(std::size_t len) = 0;

	virtual void close() =0;
	virtual bool is_chunked() const = 0;

	virtual tcp::endpoint get_host() const = 0;
};

class Asyn_HttpClient
	: public HttpClient
	, public boost::enable_shared_from_this<Asyn_HttpClient>
{
public:

	static Asyn_HttpClientPtr create(boost::asio::io_service& ios, 
		ClientHandlerPtr phandler, HttpRequest::HttpRequestPtr prequest);

public:

	bool set_request(HttpRequest::HttpRequestPtr prequest);

	void connect();

	void request();

	void request(HttpRequest::HttpRequestPtr prequest);

	void read_chunk();

	void read_content(std::size_t len);

	void close();

	bool is_chunked() const { return m_is_chunk; }

	tcp::endpoint get_host() const {return m_endpoint ; }

private:
	Asyn_HttpClient(boost::asio::io_service& ios, 
		ClientHandlerPtr phandler,
		HttpRequest::HttpRequestPtr prequest);

	void handle_resolve(const boost::system::error_code& ec, 
		tcp::resolver::iterator ep_it);

	void handle_connect(const boost::system::error_code& ec, 
		tcp::resolver::iterator ep_it);

	void handle_write(const boost::system::error_code& ec, size_t trans_bytes);

	void handle_read_header(const boost::system::error_code& ec, size_t trans_bytes);

	void handle_read_content(const boost::system::error_code& ec, 
		size_t trans_bytes, size_t need_len, int64_t data_file_off, int64_t data_content_off);
private:
	///
	/// TODO: to be completed
	///
	void read_chunk_size();
	void handle_read_chunk_size(const boost::system::error_code& ec, size_t trans_bytes);
    
    void read_chunk_body();
    void handle_read_chunk_body(const boost::system::error_code& ec, size_t trans_bytes);

private:
	tcp::endpoint m_endpoint;
	tcp::endpoint m_ep;
	tcp::socket m_socket;
	tcp::resolver m_resolver;

	tcp::endpoint m_target_endpoint;
	std::string m_target_host;
	unsigned int m_target_port;

	HttpRequest::HttpRequestPtr m_request;
	HttpResponse::HttpResponsePtr m_response;

	boost::asio::streambuf m_response_buf;
	//    boost::asio::streambuf m_request_buf;
	int64_t m_content_len;
	int64_t m_content_off;
	int64_t m_file_off;

	bool m_is_chunk;
    uint32_t m_last_chunk_size;

	ClientHandlerPtr m_client_handler;

	bool m_is_closed;
};


class Syn_HttpClient
	: public HttpClient
	, public boost::enable_shared_from_this<Syn_HttpClient>
{
public:

	static Syn_HttpClientPtr create(boost::asio::io_service& ios, 
		ClientHandlerPtr phandler, HttpRequest::HttpRequestPtr prequest);

public:

	bool set_request(HttpRequest::HttpRequestPtr prequest);

	void connect();

	void request();

	void request(HttpRequest::HttpRequestPtr prequest);

	void read_chunk();

	void read_content(std::size_t len);

	void close();

	bool is_chunked() const { return m_is_chunk; }

	tcp::endpoint get_host() const {return m_endpoint ; }

private:
	Syn_HttpClient(boost::asio::io_service& ios, 
		ClientHandlerPtr phandler,
		HttpRequest::HttpRequestPtr prequest);

	void handle_resolve(const boost::system::error_code& ec, 
		tcp::resolver::iterator ep_it);

	void handle_connect(const boost::system::error_code& ec, 
		tcp::resolver::iterator ep_it);

	void handle_write(const boost::system::error_code& ec, size_t trans_bytes);

	void handle_read_header(const boost::system::error_code& ec, size_t trans_bytes);

	void handle_read_content(const boost::system::error_code& ec, 
		size_t trans_bytes, size_t need_len, int64_t data_file_off, int64_t data_content_off);
private:
	///
	/// TODO: to be completed
	///
	void read_chunk_size();
	void handle_read_chunk_size(const boost::system::error_code& ec, size_t trans_bytes);

private:
	tcp::endpoint m_endpoint;
	tcp::socket m_socket;
	tcp::resolver m_resolver;

	tcp::endpoint m_target_endpoint;
	std::string m_target_host;
	unsigned int m_target_port;

	HttpRequest::HttpRequestPtr m_request;
	HttpResponse::HttpResponsePtr m_response;

	boost::asio::streambuf m_response_buf;
	//    boost::asio::streambuf m_request_buf;
	int64_t m_content_len;
	int64_t m_content_off;
	int64_t m_file_off;

	bool m_is_chunk;

	ClientHandlerPtr m_client_handler;

	bool m_is_closed;
};

HttpClientPtr HttpClientFactory(boost::asio::io_service& ios, ClientHandlerPtr phandler, 
	HttpRequest::HttpRequestPtr prequest,bool is_asyn = true );

#endif 
