#ifndef __LOCAL_LOCAL_HTTP_SERVER_H__
#define __LOCAL_LOCAL_HTTP_SERVER_H__

#include <boost/thread/once.hpp>
#include "../p2pcommon/http_server.h"
#include "local_http_connection.h"
#include "../p2pcommon/base/common.h"
#include "../include/ip2p_localserver.h"
#include <map>
#include "rapidxml/shxmlparser.hpp"

using namespace std;
using boost::asio::ip::tcp;

class LocalHttpConnection;
//typedef std::map<boost::shared_ptr<tcp::socket>,std::pair<boost::shared_ptr<LocalHttpConnection>, sh_int_64 > >	SHConnectionMap;
typedef std::map<boost::shared_ptr<LocalHttpConnection>, std::pair<sh_int_64, sh_int_32> >	SHConnectionMap;

typedef std::map<boost::shared_ptr<LocalHttpConnection>, sh_int_64 >	sh_connection_uid_map;

class LocalHttpServer
    : private boost::noncopyable
    , public boost::enable_shared_from_this<LocalHttpServer>
    , public HttpServerHandler
	, public IP2PDownloadSink
{
public:
    static boost::shared_ptr<LocalHttpServer> Inst()
    {
        boost::call_once(LocalHttpServer::init_it, ms_once_flag);
        return ms_pinst;
    }

	boost::asio::io_service& get_io_service(int thread_id)
	{
		return ios_pool_->get_ios(thread_id);
	}

public:
    ~LocalHttpServer();

	bool start(SHP2PSystemParam system_param);
    void stop(void);
	void on_connection_close(boost::shared_ptr<LocalHttpConnection> con, int call_id);
	void on_connection_close(sh_int_64 uid, boost::shared_ptr<LocalHttpConnection> sp_connection);
	void on_connection_update(sh_int_64 uid, int call_id, boost::shared_ptr<LocalHttpConnection> sp_connection);
	void on_stop_request_video_data(boost::shared_ptr<LocalHttpConnection> con);

public: /*实现HttpServerHandler接口*/
    virtual void on_accept(const boost::system::error_code& ec, boost::shared_ptr<tcp::socket> p_socket);
	
public: /*实现IP2PDownloadSink, p2p回调接口*/
	
	//p2p异常处理
	virtual void on_p2p_happen_error(sh_int_64 unique_id, sh_uint_32 index, enum SHP2pErrorType error_type, sh_uint_32 error_code, sh_uint_32 status_code);

	//接收p2p数据
	virtual void on_recv_p2p_video_data(sh_int_64 unique_id, sh_uint_32 index, sh_int_8_p data, sh_uint_32 len, bool isheader, sh_int_32 real_mp4_size, sh_int_32 cur_download_pos, int32_t call_id);

	//下载mp4数据完毕
	virtual void on_finish_p2p_video_data(sh_int_64 unique_id);

	//注册ID
	virtual void on_get_register_id(sh_int_32 id, sh_char_p sohu_key);

public:  /*向加速器请求数据接口*/
	
	//播放通过mp4段请求数据
	sh_int_64 start_request_video_data(sh_int_32 vid, enum SHVideoClarity clarity, sh_int_32 index, bool is_mytv, sh_int_32 pnum);

	//停止下载mp4数据
	void stop_request_video_data(sh_int_64 unique_id);

	//暂停下载
	void pause_request_video_data(sh_int_64 unique_id);

	//断点下载
	void restart_request_video_data(sh_int_64 unique_id);

	//range请求
	sh_int_64 start_play_video_data_range(sh_int_32 vid, enum SHVideoClarity clarity, bool is_mytv, sh_int_32 start_range, sh_int_32 end_range, sh_int_64 call_id);

	//离线下载
	sh_int_64 start_download_video_data_range(sh_int_32 vid, enum SHVideoClarity clarity, bool is_mytv, sh_int_32 start_range, sh_int_32 end_range, sh_int_64 call_id);

	//获取离线下载信息
	bool get_download_info(sh_int_64 unique_id, NewSHDispInfo* sh_disp_info);

    //设置是否开启上传
    void   set_allow_share(bool allow_share);
public: 
	/*localserver限制存放1段mp4数据大小*/
	bool is_request_section_completed(void) {return section_completed_;}
	sh_int_32 get_downloaded_pos(void) {return downloaded_len_;}
	
	//获取视频总长度
	sh_int_32 get_video_total_lens(sh_int_64 uid);

	std::tstring get_log_path(void) {return log_path_;}

	sh_int_32 get_call_id_num(void);

private:
    LocalHttpServer();
	bool init(unsigned int local_tcp_port);
	void uninit(void);
	void init_member(void);
	bool register_log_file(std::tstring str_path);
	
	boost::shared_ptr<LocalHttpConnection> get_http_connection(sh_int_64 unique_id, int call_id);
	void send_data(boost::shared_ptr<LocalHttpConnection> sp_conn, std::string &data, int len, bool isheader, sh_int_32 real_mp4_size, sh_int_32 cur_download_pos);
	void execute_on_accepted(const boost::system::error_code& ec, boost::shared_ptr<tcp::socket> p_socket);

	void recv_p2p_video_data(sh_int_64 unique_id, sh_uint_32 index, std::string data, sh_uint_32 len, bool isheader, sh_int_32 real_mp4_size, sh_int_32 cur_download_pos, int32_t call_id);
	void finish_p2p_video_data(sh_int_64 unique_id);

    static void init_it()
    {
        ms_pinst.reset(new LocalHttpServer);
    }

	bool WritePrivateProfileString(const char* lpAppName, const char* lpKeyName, const char* lpString, const char* lpFileName);
	sh_int_32 GetProfileInt(const char* lpAppName, const char* lpKeyName, const char* lpFileName);

	std::string get_config_file(void);

    static boost::shared_ptr<LocalHttpServer> ms_pinst;
    static boost::once_flag ms_once_flag;
	static boost::shared_ptr<IOServicePool> ios_pool_;

private:
    SHConnectionMap sock_connection_map_; 

    boost::shared_ptr<HttpServer> http_server_;
    bool is_running_;
	SHP2pSystemNofity p2p_notify_;
	std::map<sh_int_64, DOWNLOADVIDEOINFO> download_info_;
	std::tstring log_path_;

	std::map<sh_int_64, sh_int_32> map_video_lens_;

	boost::shared_ptr<IP2PDownload> m_sp_p2p_;
	sh_xml_document<sh_xml_encode_type_utf8> m_rapidxmlDoc;
	
	//限制存放1段mp4数据大小
	unsigned char request_index_;  //正在下载播放时请求需要的段
	sh_int_32 downloaded_len_;  
	bool section_completed_;
	sh_int_32 recv_counter_;
	bool is_find_index_;
	sh_int_32 off_downloaded_len_;
	boost::mutex  mutex_;
	boost::mutex  call_id_mutex_;

#ifdef READ_LOCAL_FILE
	FILE *local_file_;
	int local_total_len_;
	int cur_file_pos_;
#endif

};

#endif // __LOCAL_LOCAL_HTTP_SERVER_H__