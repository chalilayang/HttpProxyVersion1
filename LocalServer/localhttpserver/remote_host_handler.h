/*
 * remote_host_handler.h
 *
 *  Created on: 2014-10-20
 *      Author: chalilayang
 */

#ifndef REMOTE_HOST_HANDLER_H_
#define REMOTE_HOST_HANDLER_H_
#include "../p2pcommon/base/common.h"
#include "../p2pcommon/http_request.h"
#include "../p2pcommon/http_response.h"

using boost::asio::ip::tcp;
class RemoteHostHandler;
typedef boost::shared_ptr<RemoteHostHandler> RemoteHostHandlerPtr;

struct RemoteHostHandlerInterface {
	virtual ~RemoteHostHandlerInterface() {
	}
	virtual void on_remote_host_data_recieved(const IOBuffer& io_buf) = 0;
};

class RemoteHostHandler: private boost::noncopyable,
		public boost::enable_shared_from_this<RemoteHostHandler> {
public:
	static boost::shared_ptr<RemoteHostHandler> create(
			boost::asio::io_service& ios,
			boost::shared_ptr<RemoteHostHandlerInterface> phandler);
	RemoteHostHandler(boost::asio::io_service& ios,
			boost::shared_ptr<RemoteHostHandlerInterface> phandler);
	void start_get_remote_host_data(HttpRequest::HttpRequestPtr p_request);
	inline bool hasFinished() {
		return has_finished;
	}
	virtual ~RemoteHostHandler();
private:
	void handle_resolve(const boost::system::error_code& err,
			tcp::resolver::iterator endpoint_iterator);
	void handle_connect(const boost::system::error_code& err,
			tcp::resolver::iterator endpoint_iterator);
	void handle_write_request(const boost::system::error_code& err);
	void handle_read_remote_data(const boost::system::error_code& err,
			size_t trans_bytes);

private:
	bool has_finished;
	HttpRequest::HttpRequestPtr request_ptr;
	tcp::resolver resolver_;
	tcp::socket remote_socket_;
	boost::asio::streambuf request_;
	boost::asio::streambuf response_;
	boost::weak_ptr<RemoteHostHandlerInterface> m_server_handler;
};

#endif /* REMOTE_HOST_HANDLER_H_ */
