//  local_http_server.h
/***
 *  响应http请求，建立并管理LocalHttpConnection
 */

#ifndef _KERNEL_LOCAL_LOCAL_HTTP_SERVER_H_
#define _KERNEL_LOCAL_LOCAL_HTTP_SERVER_H_

#include <boost/thread/once.hpp>
#include "../p2pcommon/http_server.h"
#include "local_http_connection.h"
#include "../p2pcommon/base/common.h"
#include "../include/ip2p_localserver.h"
#include <map>

using namespace std;
using boost::asio::ip::tcp;

class LocalHttpConnection;
typedef std::map<boost::shared_ptr<tcp::socket>,std::pair<boost::shared_ptr<LocalHttpConnection>, sh_int_64 > >	SHConnectionMap;
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

public:
    ~LocalHttpServer();

	bool start(SHP2PSystemParam system_param);
    void stop(void);

    virtual void on_accept(const boost::system::error_code& ec, boost::shared_ptr<tcp::socket> p_socket);
	void on_connection_close(boost::shared_ptr<LocalHttpConnection> con);
	void on_connection_start(sh_int_64 uid, boost::shared_ptr<LocalHttpConnection> sp_connection);

	/*实现IP2PDownloadSink, p2p回调接口*/
	//p2p异常处理
	virtual void on_p2p_happen_error(sh_int_64 unique_id, sh_uint_32 index, enum SHP2pErrorType error_type, sh_uint_32 error_code, sh_uint_32 status_code);

	//接收p2p数据
	virtual void on_recv_p2p_video_data(sh_int_64 unique_id, sh_uint_32 index, sh_int_8_p data, sh_uint_32 len, bool isheader, sh_int_32 real_mp4_size);

	//下载mp4数据完毕
	virtual void on_finish_p2p_video_data(sh_int_64 unique_id);

	/*向加速器请求数据 */
	sh_int_64 start_request_video_data(sh_int_32 vid, enum SHVideoClarity clarity, sh_int_32 index, bool is_mytv, sh_int_32 pnum);

	

private:
    LocalHttpServer();
	bool init(unsigned int local_tcp_port);
	void uninit(void);
	bool register_log_file(std::tstring str_path);
	boost::shared_ptr<LocalHttpConnection> get_http_connection(sh_int_64 unique_id);
	void send_data(boost::shared_ptr<LocalHttpConnection> sp_conn, std::string &data, int len, bool isheader, sh_int_32 real_mp4_size);

    static void init_it()
    {
        ms_pinst.reset(new LocalHttpServer);
    }

    static boost::shared_ptr<LocalHttpServer> ms_pinst;
    static boost::once_flag ms_once_flag;
	static boost::shared_ptr<IOServicePool> ios_pool_;

private:
    SHConnectionMap sock_connection_map_; 
	sh_connection_uid_map connection_uid_map_;

    boost::shared_ptr<HttpServer> http_server_;
    bool is_running_;
	SHP2pSystemNofity p2p_notify_;

	boost::shared_ptr<IP2PDownload> m_spP2pInterface;
};

#endif // _KERNEL_LOCAL_LOCAL_HTTP_SERVER_H_