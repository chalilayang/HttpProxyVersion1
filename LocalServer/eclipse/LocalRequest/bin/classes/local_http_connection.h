//  local_http_connection.h
//  Copyright (c) 2010 qiyi.com. All rights reserved.

#ifndef _KERNEL_LOCAL_LOCAL_HTTP_CONNECTION_H_
#define _KERNEL_LOCAL_LOCAL_HTTP_CONNECTION_H_

#include "Utils.h"
#include "../p2pcommon/base/common.h"
#include "../include/player_def.h"
#include "../p2pcommon/http_service.h"


#define FLASH_CROSS_DOMAIN_HEADER	"<cross-domain-policy><allow-access-from domain=\"*\"to-ports=\"*\"/></cross-domain-policy>\0"

using boost::asio::ip::tcp;

class LocalHttpServer;
class LocalHttpConnection
    : private boost::noncopyable
    , public boost::enable_shared_from_this<LocalHttpConnection>
    , public HttpServiceHandler
{

public:
	static boost::shared_ptr<LocalHttpConnection> Create(boost::shared_ptr<LocalHttpServer> localserver);

public:
    ~LocalHttpConnection();
	
	void Init();

    void Open(boost::shared_ptr<tcp::socket> psock);

    void Close();

    virtual void on_service_close();

    virtual void on_service_read(const boost::system::error_code& ec, HttpRequest::HttpRequestPtr p_request, IOBuffer content_buf);

    virtual void on_service_read_timeout();

    virtual void on_service_write(const boost::system::error_code& ec, size_t trans_bytes);

	void response200_plain(std::string &str_msg);

	void response200_xml(std::string &str_msg);

	bool parse_flash_request(string data);

	bool parse_notify_buffer(string data,string& uid,int& canPlayTime);

	void start_download();
	void send_data(std::string &str_data, int len, bool isheader = false, sh_int_32 real_mp4_size = 0);

private:
    LocalHttpConnection(boost::shared_ptr<LocalHttpServer> localserver);
	SHVideoClarity get_video_definiton(VideoVersion definiton);

private:
	bool is_open_;
	std::map<string,int32_t> userid_uid_map_;
	boost::shared_ptr<HttpService> http_service_;
	DownloadInfo download_info_;
	boost::weak_ptr<LocalHttpServer> local_http_server_;
	int32_t erro_count_;

};

#endif // _KERNEL_LOCAL_LOCAL_HTTP_CONNECTION_H_