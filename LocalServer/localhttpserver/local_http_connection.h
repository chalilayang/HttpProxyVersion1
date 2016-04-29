#ifndef __LOCAL_HTTP_CONNECTION_H__
#define __LOCAL_HTTP_CONNECTION_H__

#include "Utils.h"
#include "../p2pcommon/base/common.h"
#include "../include/macro_define.h"
#include "../p2pcommon/http_service.h"
#include "remote_host_handler.h"


#define FLASH_CROSS_DOMAIN_HEADER	"<cross-domain-policy><allow-access-from domain=\"*\"to-ports=\"*\"/></cross-domain-policy>\0"

using boost::asio::ip::tcp;

class LocalHttpServer;
class LocalHttpConnection
    : private boost::noncopyable
    , public boost::enable_shared_from_this<LocalHttpConnection>
    , public RemoteHostHandlerInterface
    , public CHttpServiceHandler
{

public:
	static boost::shared_ptr<LocalHttpConnection> Create(boost::shared_ptr<LocalHttpServer> localserver);

public:
    ~LocalHttpConnection();
	
	void Init(void);
    void Open(boost::shared_ptr<tcp::socket> psock);
    void Close(void);

public:	/*实现HttpServiceHandler接口*/
    virtual void on_service_close(void);
    virtual void on_service_read(const boost::system::error_code& ec, HttpRequest::HttpRequestPtr p_request, IOBuffer content_buf);
    virtual void on_service_read_timeout(void);
    virtual void on_service_write(const boost::system::error_code& ec, size_t trans_bytes);
	virtual void on_service_request_video_data(int buffer_size);
public:	/*RemoteHostHandlerInterface接口*/
	virtual void on_remote_host_data_recieved(const IOBuffer& io_buf);
public:
	//向上层发送数据
	void send_data(std::string &str_data, int len, bool isheader = false, sh_int_32 real_mp4_size = 0, sh_int_32 cur_download_pos = -2);
	
	SHDOWNLOAD_TYPE get_download_type(void) {return download_type_;}

	//boost::shared_ptr<tcp::socket>& get_socket(void) {return http_service_->get_socket();}
	/*boost::shared_ptr<CHttpService>& get_http_service(void) {return http_service_;}*/
	bool is_connection_close(void) {return !is_open_;}

	bool is_buffer_empty(void);

private:
    LocalHttpConnection(boost::shared_ptr<LocalHttpServer> localserver);
	
	//离线下载
	bool response_get_offline_download_info(void);
	bool response_pause_download_task(void);
	bool response_restart_download_task(void);
	bool response_delete_download_task(void);

	void response_200_ok_plain(std::string &str_msg);
	void response200_xml(std::string &str_msg);
	bool response_get_p2p_reported_param(void);

	//解析上层http请求信息
	bool parse_http_request(std::string data);

	//http请求处理
	void process_http_request(void);
	//狐播http请求处理
	void process_foxplay_request(std::string data);
	
	//响应seek操作
	void send_seek_header(void);

	uint64_t get_tickcount()
	{
#ifdef WIN32
		return ::GetTickCount();
#else
		struct timeval tv;
		gettimeofday(&tv, 0);
		return tv.tv_sec * 1000 + tv.tv_usec / 1000;
#endif
	}

	std::string get_log_filename(void);
	std::string get_log_path(void);

private:
	bool is_open_;
	boost::shared_ptr<CHttpService> http_service_;
	HttpRequestInfo request_info_;
	boost::weak_ptr<LocalHttpServer> local_http_server_;
	int32_t erro_count_;
	uint64_t m_last_timeout_;

	SHDOWNLOAD_TYPE download_type_;
	RemoteHostHandlerPtr remote_host_handler_ptr;

#ifdef WRITE_FILE_POS
	FILE *m_file_; //只限于播放拖拽时保存mp4数据
#endif
};

#endif // __LOCAL_HTTP_CONNECTION_H__
