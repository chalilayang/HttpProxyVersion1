/*
 * remote_host_handler.cpp
 *
 *  Created on: 2014-10-20
 *      Author: chalilayang
 */

#include "remote_host_handler.h"
#include "../p2pcommon/base/algorithm.h"
#include "../p2pcommon/log/log.h"

boost::shared_ptr<RemoteHostHandler> RemoteHostHandler::create(
		boost::asio::io_service& ios,
		boost::shared_ptr<RemoteHostHandlerInterface> phandler) {
	return boost::shared_ptr<RemoteHostHandler>(
			new RemoteHostHandler(ios, phandler));
}

RemoteHostHandler::RemoteHostHandler(boost::asio::io_service& ios,
		boost::shared_ptr<RemoteHostHandlerInterface> phandler) :
		resolver_(ios), remote_socket_(ios), m_server_handler(phandler) {
	// TODO Auto-generated constructor stub

}

void RemoteHostHandler::start_get_remote_host_data(
		HttpRequest::HttpRequestPtr p_request) {
	if (!p_request) {
		return;
	}
	has_finished = false;
	request_ptr = p_request;
	std::string remote_host;
	unsigned int port;
	std::string port_str;
	request_ptr->get_host_port(remote_host, port);

	std::ostream request_stream(&request_);
	request_stream << request_ptr->serialize_to_string();

	try {
		port_str = boost::lexical_cast<std::string>(port);
	} catch (boost::bad_lexical_cast&) {
		port_str = "80";
	}
	// Get a list of endpoints corresponding to the server name.
	tcp::resolver::query query(remote_host, port_str);
	tcp::resolver::iterator endpoint_iterator = this->resolver_.resolve(query);
	tcp::resolver::iterator end;

	resolver_.async_resolve(query,
			boost::bind(&RemoteHostHandler::handle_resolve, shared_from_this(),
					boost::asio::placeholders::error,
					boost::asio::placeholders::iterator));
}

void RemoteHostHandler::handle_resolve(const boost::system::error_code& err,
		tcp::resolver::iterator endpoint_iterator) {
	if (!err) {
		//LOGI("RemoteHostHandler::async_connect\n");
		// Attempt a connection to the first endpoint in the list. Each endpoint
		// will be tried until we successfully establish a connection.
		tcp::endpoint endpoint = *endpoint_iterator;
		remote_socket_.async_connect(endpoint,
				boost::bind(&RemoteHostHandler::handle_connect,
						shared_from_this(), boost::asio::placeholders::error,
						++endpoint_iterator));
	}
}

void RemoteHostHandler::handle_connect(const boost::system::error_code& err,
		tcp::resolver::iterator endpoint_iterator) {
	if (!err) {
		//LOGI("RemoteHostHandler::async_write");
		// The connection was successful. Send the request.
		boost::asio::async_write(remote_socket_, request_,
				boost::bind(&RemoteHostHandler::handle_write_request,
						shared_from_this(), boost::asio::placeholders::error));
	} else if (endpoint_iterator != tcp::resolver::iterator()) {
		// The connection failed. Try the next endpoint in the list.
		remote_socket_.close();
		tcp::endpoint endpoint = *endpoint_iterator;
		remote_socket_.async_connect(endpoint,
				boost::bind(&RemoteHostHandler::handle_connect,
						shared_from_this(), boost::asio::placeholders::error,
						++endpoint_iterator));
	}
}
void RemoteHostHandler::handle_write_request(
		const boost::system::error_code& err) {
	if (!err) {
		// Read the response status line. The response_ streambuf will
		// automatically grow to accommodate the entire line. The growth may be
		// limited by passing a maximum size to the streambuf constructor.
		boost::asio::async_read_until(this->remote_socket_, response_, "\r\n",
				boost::bind(&RemoteHostHandler::handle_read_remote_data,
						shared_from_this(), boost::asio::placeholders::error,
						boost::asio::placeholders::bytes_transferred));
	}
}

void RemoteHostHandler::handle_read_remote_data(
		const boost::system::error_code& err, size_t trans_bytes) {
	if (!err) {
		if (this->m_server_handler.expired()) {
			return;
		}

		std::ostringstream header_reponse_stream;
		header_reponse_stream << &response_;
		std::vector<std::string> lines;
		Splite(lines, header_reponse_stream.str(), "\r\n\r\n");

		IOBuffer io_buf(lines[0] + "\r\n\r\n");
		HttpResponse http_reponse_header(io_buf);
		if (HttpResponse::SH_HTTP_STATUS_OK
				== http_reponse_header.get_status_code()) {
//			this->m_server_handler.lock()->on_remote_host_data_recieved(io_buf);

			// **************************************************************
			// 取出数据->转发数据
			// **************************************************************
			boost::system::error_code system_error_;
			boost::asio::streambuf response_body;
			std::ostringstream packetStream;
			packetStream << lines[0];
			packetStream << "\r\n\r\n";
			if (lines.size() > 1) {
				packetStream << lines[1];
			}
			while (boost::asio::read(this->remote_socket_, response_body,
					boost::asio::transfer_at_least(1), system_error_)) {
				packetStream << &response_body;
			}
			std::string str_msg = packetStream.str();
			if (!str_msg.empty()) {
				IOBuffer msg(str_msg);
				if (!this->m_server_handler.expired()) {
					this->m_server_handler.lock()->on_remote_host_data_recieved(msg);
				}
			}
			has_finished = true;
		} else if (HttpResponse::SH_HTTP_STATUS_MOVED
				== http_reponse_header.get_status_code()) {

		}
	}
}

RemoteHostHandler::~RemoteHostHandler() {
	// TODO Auto-generated destructor stub
}

