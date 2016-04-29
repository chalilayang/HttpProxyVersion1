#include "local_http_server.h"
#include "../p2pcommon/log/log.h"
#include "../include/p2p_export.h"
#include "../p2pcommon/base/file.h"
#include "../p2pcommon/base/algorithm.h"
#ifdef WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

boost::shared_ptr<LocalHttpServer> LocalHttpServer::ms_pinst;
boost::once_flag LocalHttpServer::ms_once_flag = BOOST_ONCE_INIT;
boost::shared_ptr<IOServicePool> LocalHttpServer::ios_pool_;

LocalHttpServer::LocalHttpServer() : is_running_(false)
{
	m_sp_p2p_.reset(get_p2p_instance()); 
	if (m_sp_p2p_.get())
	{
		m_sp_p2p_->advice(this);
	}

	memset(&p2p_notify_, 0, sizeof(p2p_notify_));
	init_member();	
}

LocalHttpServer::~LocalHttpServer()
{
	INFO_LOG(LOCAL_SERVER_LOG,_T("localserver 已析构了!\n"));

	if (m_sp_p2p_.get())
	{
		m_sp_p2p_->un_advice(this);
		m_sp_p2p_.reset();
	}

	download_info_.clear();
}

void LocalHttpServer::init_member(void)
{
	download_info_.clear();
	
	request_index_ = -1;
	downloaded_len_ = 0;
	section_completed_ = false;
	recv_counter_ = 0;
	is_find_index_ = false;

	off_downloaded_len_ = 0;

#ifdef READ_LOCAL_FILE
	local_file_ = NULL;
	local_total_len_ = 0;
	cur_file_pos_ = 0;
#endif
}

bool LocalHttpServer::init(unsigned int local_tcp_port)
{
    if (is_running_) return false;

	boost::asio::ip::address addr = boost::asio::ip::address::from_string("127.0.0.1");
    tcp::endpoint ep(addr,local_tcp_port);

	if (!ios_pool_)
	{
		ios_pool_.reset(new IOServicePool(K_THREAD_SUM));
	}
	ios_pool_->start();

    http_server_ = HttpServer::create(ios_pool_->get_ios(K_THREAD_LISTEN), ep, local_tcp_port, shared_from_this());

    if (http_server_->start())
    {
        INFO_LOG(LOCAL_SERVER_LOG, _T("HttpServer start succeed at %d\n"), http_server_->get_port());
		http_server_->accept_one();
    }
    else
    {
        INFO_LOG(LOCAL_SERVER_LOG, _T("HttpServer start failed at %d\n"), http_server_->get_port());
		return false;
    }

    is_running_ = true;

	return true;
}

bool LocalHttpServer::start(SHP2PSystemParam system_param)
{
	if (is_running_)
	{
		return true;
	}

	log_path_ = system_param.log_path;
	system_param.register_id = GetProfileInt("SOHU_P2P", "RegisterId", get_config_file().c_str());
	if (m_sp_p2p_.get())
	{
		m_sp_p2p_->init_p2p_system(system_param);
	}

	std::tstring str_tmp(system_param.log_path);
	register_log_file(str_tmp);
	INFO_LOG(LOCAL_SERVER_LOG,_T("平台 = %d\n"), system_param.platform_type);

#ifdef READ_LOCAL_FILE
	std::tstring str_path = log_path_;

#ifdef WIN32
	if (str_path[str_path.size()-1] != _T('\\'))
	{
		str_path += _T("\\");
	}	
#else
	if (str_path[str_path.size()-1] != '/')
	{
		str_path += "/";
	}	
#endif
	str_path += _T("1.mp4");

	if (local_file_ == NULL)
	{
		INFO_LOG(LOCAL_SERVER_LOG, _T("open local file : %s\r\n"), str_path.c_str());
		local_file_ = fopen(w2b(str_path).c_str(), "rb");
		if (local_file_ == NULL)
		{
			INFO_LOG(LOCAL_SERVER_LOG, _T("open local file fail! %s\r\n"), str_path.c_str());
		}
	}

	fseek(local_file_, 0, SEEK_END);
	local_total_len_ = ftell(local_file_);
	fseek(local_file_, 0, SEEK_SET);
#endif

	return init(8834);
}

void LocalHttpServer::stop(void)
{
	INFO_LOG(LOCAL_SERVER_LOG,_T("上层调用了localserver中stop!\n"));

	if (m_sp_p2p_.get())
	{
		m_sp_p2p_->un_advice(this);
		m_sp_p2p_->uninit_p2p_system();
		m_sp_p2p_.reset();
	}

	uninit();
}

void LocalHttpServer::uninit(void)
{

	INFO_LOG(LOCAL_SERVER_LOG,_T("localserver uninit 反初始化!\n"));

    if (false == is_running_) return;

    if (http_server_)
    {
        http_server_->stop();
        http_server_.reset();
    }

	ios_pool_->stop();
	ios_pool_.reset();
	sock_connection_map_.clear();
    is_running_ = false;
}

void LocalHttpServer::on_accept(const boost::system::error_code& ec, boost::shared_ptr<tcp::socket> p_socket)
{
	if (false == is_running_) return;

	execute_on_accepted(ec, p_socket);

	/*if (ios_pool_)
	{
		ios_pool_->get_ios(K_THREAD_SEND).post(boost::bind(&LocalHttpServer::execute_on_accepted, LocalHttpServer::Inst(), ec, p_socket));
	}	*/
}

void LocalHttpServer::execute_on_accepted(const boost::system::error_code& ec, boost::shared_ptr<tcp::socket> p_socket)
{
	if (false == is_running_) return;

	boost::mutex::scoped_lock lock(mutex_);
// 	if (sock_connection_map_.find(p_socket) != sock_connection_map_.end())
// 	{
// 		INFO_LOG(LOCAL_SERVER_LOG,_T("存在相同的 socket map has this socket 0x%08x\n"), p_socket.get());
// 		sock_connection_map_.erase(p_socket);
// 	}

	boost::shared_ptr<LocalHttpConnection> connection = LocalHttpConnection::Create(shared_from_this());
	connection->Open(p_socket);
	sock_connection_map_.insert(std::make_pair(connection, std::make_pair(-1, 0)));

	INFO_LOG(LOCAL_SERVER_LOG,_T("收到HTTP请求, socket=0x%08x, size=%d\r\n"), p_socket.get(), sock_connection_map_.size());
}

void LocalHttpServer::on_stop_request_video_data(boost::shared_ptr<LocalHttpConnection> con)
{
	if (false == is_running_) return;

	boost::mutex::scoped_lock lock(mutex_);
	SHConnectionMap::iterator it;
	for (it = sock_connection_map_.begin(); it != sock_connection_map_.end();it++)
	{
		if (it->first == con)
			break;
	}

	if (it != sock_connection_map_.end())
	{
		//停止p2p数据下载
		if (it->second.first != -1)
		{
			stop_request_video_data(it->second.first);
			INFO_LOG(LOCAL_SERVER_LOG, _T("停止p2p mp4数据下载,  uid = %lld\r\n"), it->second.first);
		}
	}    
}

void LocalHttpServer::on_connection_close(boost::shared_ptr<LocalHttpConnection> con, sh_int_64 call_id)
{
	if (false == is_running_) return;

	boost::mutex::scoped_lock lock(mutex_);
	SHConnectionMap::iterator it;
	for (it = sock_connection_map_.begin(); it != sock_connection_map_.end();it++)
	{
		if (it->first == con && it->second.second == call_id)
			break;
	}

	if (it != sock_connection_map_.end())
	{
		//INFO_LOG(LOCAL_SERVER_LOG, _T("on_connection_close,uid=%d, size=%d\r\n"), it->second.first, sock_connection_map_.size());
		//stop_request_video_data(it->second.first); // for vlc compac

		INFO_LOG(LOCAL_SERVER_LOG, _T("从 map 中删除 connection, size=%d, call_id=%d\r\n"), sock_connection_map_.size(), it->second.second);
		sock_connection_map_.erase(it);
	}   
}

void LocalHttpServer::on_connection_close(sh_int_64 uid, boost::shared_ptr<LocalHttpConnection> sp_connection)
{
	if (false == is_running_) return;

	boost::mutex::scoped_lock lock(mutex_);
	SHConnectionMap::iterator it;
	for (it = sock_connection_map_.begin(); it != sock_connection_map_.end();)
	{
		if (it->second.first == uid && it->first.get() != NULL && it->first.get() != sp_connection.get())
		{
			//如果有相同的uid,要先关闭掉原来的connection(从同一个连接发请求除外),一个uid可能会对应多个Conection
			INFO_LOG(LOCAL_SERVER_LOG, _T("停止原来uid数据段下载，并且删除已有的 connection,  uid=%lld, size=%d\r\n"), uid, sock_connection_map_.size());
			sock_connection_map_.erase(it++);
		}
		else if (it->second.first != -1 && it->second.first != uid && it->first.get() != NULL && it->first->get_download_type() == SHDOWNLOAD_PLAY)
		{
			//播放有且只有一个connection
			INFO_LOG(LOCAL_SERVER_LOG, _T("停止原来播放,uid=%lld, size=%d\r\n"), uid, sock_connection_map_.size());
			stop_request_video_data(it->second.first);
			it++;
		}
		else
		{
			it++;
		}
	}
}

void LocalHttpServer::on_connection_update(sh_int_64 uid, sh_int_64 call_id, boost::shared_ptr<LocalHttpConnection> sp_connection)
{
	if (false == is_running_) return;

	boost::mutex::scoped_lock lock(mutex_);
	SHConnectionMap::iterator it;

	//播放只有一个uid
	for (it = sock_connection_map_.begin(); it != sock_connection_map_.end();)
	{
		if (it->first.get() != NULL && it->first.get() != sp_connection.get() 
			&& it->first->get_download_type() == SHDOWNLOAD_PLAY && it->second.first != uid)
		{
			INFO_LOG(LOCAL_SERVER_LOG, _T("停止并删除原来的播放,old_uid=%lld, size=%d\r\n"), it->second.first, sock_connection_map_.size());
			stop_request_video_data(it->second.first);
			sock_connection_map_.erase(it++);
		}
		else
		{
			it++;
		}
	}

	//如果有相同的uid,要先关闭掉原来的connection
 	for (it = sock_connection_map_.begin(); it != sock_connection_map_.end();)
 	{
 		if (it->second.first == uid && it->first.get() != NULL && it->first.get() != sp_connection.get() && it->first->get_download_type() == SHDOWNLOAD_OFFLINE)
 		{
			INFO_LOG(LOCAL_SERVER_LOG, _T("删除已有的 connection,  uid=%lld, size=%d, callid = %lld\r\n"), uid, sock_connection_map_.size(), it->second.second);
			//it = sock_connection_map_.erase(it); //andriod 上编译不过
			it->first->Close();
			sock_connection_map_.erase(it++);
			//it++;
 		}
		else
		{
			it++;
		}
 	}

	//保存正在下载的uid (一个uid可能会对应多个Connection)
	for (it = sock_connection_map_.begin(); it != sock_connection_map_.end(); it++)
	{
		if (it->first == sp_connection)
		{
			INFO_LOG(LOCAL_SERVER_LOG, _T("覆盖原有callid和uid,old_callid=%lld, newcall_id=%lld\r\n"), it->second.second, call_id);
			it->second.first = uid;
			it->second.second = call_id;

		}
	}
}

void LocalHttpServer::on_p2p_happen_error(sh_int_64 unique_id, sh_uint_32 index, enum SHP2pErrorType error_type, sh_uint_32 error_code, sh_uint_32 status_code)
{
	if (false == is_running_) return;

	/*boost::shared_ptr<LocalHttpConnection> sp_connection = get_http_connection(unique_id);
	if (sp_connection.get() == NULL)
	{
		return;
	}*/

	switch (error_type)
	{
	case kSHP2pError_HotVers:
		break;
	case kSHP2pError_Dispatch:
		break;
	case kSHP2pError_VideoRequest:
		break;
	default:
		break;		
	}
}

void LocalHttpServer::on_recv_p2p_video_data(sh_int_64 unique_id, sh_uint_32 index, sh_int_8_p data, sh_uint_32 len, bool isheader, sh_int_32 real_mp4_size, sh_int_32 cur_download_pos, sh_int_64 call_id)
{
	if (!isheader && len <= 0)
	{
		int a = 0;
	}

 	if (ios_pool_)
 	{
		ios_pool_->get_ios(K_THREAD_SEND).post(boost::bind(&LocalHttpServer::recv_p2p_video_data, LocalHttpServer::Inst(), unique_id, index, std::string(data,len), len, isheader, real_mp4_size, cur_download_pos, call_id));
 	}
}

void LocalHttpServer::recv_p2p_video_data(sh_int_64 unique_id, sh_uint_32 index, std::string data, sh_uint_32 len, bool isheader, sh_int_32 real_mp4_size, sh_int_32 cur_download_pos, sh_int_64 call_id)
{
	if (false == is_running_) return;

	if (unique_id < 0 || index < 0 || call_id < 0)
	{
		INFO_LOG(LOCAL_SERVER_LOG, _T("recv data happen error 异常!  uid = %lld, section = %d, call_id=%lld\r\n"), unique_id, index, call_id);
	}

	int size = sock_connection_map_.size();
	boost::shared_ptr<LocalHttpConnection> sp_connection = get_http_connection(unique_id, call_id);
	if (sp_connection.get() == NULL)
	{
		INFO_LOG(LOCAL_SERVER_LOG, _T("connection is NULL, uid=%lld, section=%d, pos=%d, call_id=%lld\r\n"), unique_id, index, cur_download_pos, call_id);
		return;
	}

	//离线下载不要向上层发数据
	SHDOWNLOAD_TYPE sh_type  = sp_connection->get_download_type();

	if (isheader)
	{
		off_downloaded_len_ = 0;
		//INFO_LOG(LOCAL_SERVER_LOG, _T("收到mp4头数据: %d, 视频总大小: %d, uid = %lld\r\n"), len, real_mp4_size, unique_id);
	}
    
	if (sh_type == SHDOWNLOAD_PLAY)
	{
		//播放时localserver最多存放1段mp4大小
		if (isheader)
		{
			map_video_lens_.insert(std::make_pair(unique_id, real_mp4_size));

			INFO_LOG(LOCAL_SERVER_LOG, _T("收到mp4头数据: %d, 视频总大小: %d, uid = %lld\r\n"), len, real_mp4_size, unique_id);

			//初始化(播放一个视频只会发送一次isheader=true,如果停止了,下次range请求,也不会发送头isheader=false)
			downloaded_len_ = 0;
			index = 0; //第一次发送mp4头数据时,index为最大段号
		}
		else if (section_completed_)
		{
			//return; //缓存的段已下载完成
		}
		else if (request_index_ != (unsigned char)-1 && request_index_ != index && !sp_connection->is_buffer_empty())
		{
			section_completed_ = true;
			INFO_LOG(LOCAL_SERVER_LOG, _T("localserver中缓存第 %d 段数据 已下载完成, index=%d\r\n"), request_index_, index);
			//return;
		}

		if (!is_find_index_ /*&& ++recv_counter_ > 2*/)
		{
			is_find_index_ = true;
			request_index_ = index;
			INFO_LOG(LOCAL_SERVER_LOG, _T("内存中存放的是第 %d 段 mp4 数据\r\n"), request_index_);
		}

		downloaded_len_ += len;
	}

	if (sh_type == SHDOWNLOAD_OFFLINE)
	{
		off_downloaded_len_ += len;
	}

#ifdef READ_LOCAL_FILE
	if (local_file_ == NULL)
	{
		return;
	}

	static bool bMovie = false;
	static int seek_pos = cur_download_pos;
	if (seek_pos != cur_download_pos)
	{
		bMovie = false;
		seek_pos = cur_download_pos;
		INFO_LOG(LOCAL_SERVER_LOG, _T("local file begin seek, pos=%x, uid=%lld\r\n"), cur_download_pos, unique_id);
	}

	if (isheader)
	{
		fseek(local_file_, 0, SEEK_END);
		local_total_len_ = ftell(local_file_);
		fseek(local_file_, 0, SEEK_SET);
		cur_file_pos_ = 0;
		INFO_LOG(LOCAL_SERVER_LOG, _T("local file size, size=%d, real_size=%d uid=%lld\r\n"), local_total_len_, real_mp4_size, unique_id);
	}
	else if (cur_download_pos > 0 && !bMovie)
	{
		fseek(local_file_, cur_download_pos, SEEK_SET);
		bMovie = true;
		cur_file_pos_ = cur_download_pos;
		INFO_LOG(LOCAL_SERVER_LOG, _T("in local file seek, pos=%x, uid=%lld\r\n"), cur_download_pos, unique_id);
	}
	else
	{
		fseek(local_file_, cur_file_pos_, SEEK_SET);
	}

	char *pdata = new char[len];
	int nReadLen = fread(pdata, 1, len, local_file_);
	cur_file_pos_ += nReadLen;

	std::string video_data(pdata, len);
	send_data(sp_connection, video_data, len, isheader, real_mp4_size, cur_download_pos);
	delete [] pdata;
	return;
#endif

	send_data(sp_connection, data, len, isheader, real_mp4_size, cur_download_pos);

}

void LocalHttpServer::on_finish_p2p_video_data(sh_int_64 unique_id)
{
	if (false == is_running_) return;

	if (ios_pool_)
	{
		ios_pool_->get_ios(K_THREAD_SEND).post(boost::bind(&LocalHttpServer::finish_p2p_video_data, LocalHttpServer::Inst(), unique_id));
	}	
}

void LocalHttpServer::finish_p2p_video_data(sh_int_64 unique_id)
{
	boost::mutex::scoped_lock lock(mutex_);
	boost::shared_ptr<LocalHttpConnection> sp_connection;
	SHConnectionMap::iterator it;
	for (it = sock_connection_map_.begin(); it != sock_connection_map_.end(); it++)
	{
		if (it->second.first == unique_id)
		{
			sp_connection = it->first;
			break;
		}
	}

	//播放时的connection不要析构
	SHDOWNLOAD_TYPE sh_type = SHDOWNLOAD_NONE;
	if (sp_connection.get() != NULL)
	{
		sp_connection->get_download_type();
	}

	if (it != sock_connection_map_.end() && sh_type == SHDOWNLOAD_OFFLINE)
	{
		INFO_LOG(LOCAL_SERVER_LOG, _T("下载完成, 删除已有的 connection,  uid=%lld, size=%d\r\n"), unique_id, sock_connection_map_.size());
		//sock_connection_map_.erase(it);
	}    

	INFO_LOG(LOCAL_SERVER_LOG, _T("下载完成 total_len=%d, uid=%lld, type=%d\r\n"), off_downloaded_len_, unique_id, sh_type);
}

boost::shared_ptr<LocalHttpConnection> LocalHttpServer::get_http_connection(sh_int_64 unique_id, sh_int_64 call_id)
{
	boost::mutex::scoped_lock lock(mutex_);
	boost::shared_ptr<LocalHttpConnection> sp_connection;
	SHConnectionMap::iterator it;

	for (it = sock_connection_map_.begin(); it != sock_connection_map_.end(); it++)
	{
		if (it->second.first == unique_id && it->second.second == call_id && !(it->first->is_connection_close()))
		{
			sp_connection = it->first;
			break;
		}
	}

	return sp_connection;
}

void LocalHttpServer::send_data(boost::shared_ptr<LocalHttpConnection> sp_conn, std::string &data, int len, bool isheader, sh_int_32 real_mp4_size, sh_int_32 cur_download_pos)
{
	if (false == is_running_) return;

	if (sp_conn.get() != NULL)
	{
		sp_conn->send_data(data, len, isheader, real_mp4_size, cur_download_pos);
	}
}

bool LocalHttpServer::register_log_file(std::tstring str_path)
{
	//log注册及log线程启动
	std::tstring str_file = str_path;
	std::string log_path;
	if (!str_file.empty())
	{
		//检查目录
#ifdef WIN32
		if (str_file[str_file.size()-1] != _T('\\'))
		{
			str_file += _T("\\");
		}	
#else
		if (str_file[str_file.size()-1] != '/')
		{
			str_file += "/";
		}	
		fprintf(stderr, "log_path is:%s\n", str_file.c_str());
#endif
		//不存在则创建目录
		try
		{
			if(!SH_filesystem::dir_exist(str_file))
			{
				if (!SH_filesystem::create_dir(str_file))
				{
					return false;
				}	
			}
		}
		catch (...)
		{
			return false;
		}
		log_path = w2b(str_file) + "SHP2PSystem.log";
	}
	else
	{
#ifndef WIN32
		fprintf(stderr, "log_path is empty\n");
#endif
		log_path = "SHP2PSystem.log";
	}

	LOG_INIT_AS_FILE(log_path, false);
	LOG_START();

	LOG_FILE_REG(LOCAL_SERVER_LOG, log_path);

#ifdef WIN32
	LOG_REG(LOCAL_SERVER_LOG, LogStream::dbgv);
#else
	LOG_REG(LOCAL_SERVER_LOG, LogStream::con);
#endif

	return true;
}

sh_int_64 LocalHttpServer::start_request_video_data(sh_int_32 vid, enum SHVideoClarity clarity, sh_int_32 index, bool is_mytv, sh_int_32 pnum)
{
	if (m_sp_p2p_.get())
	{
		return m_sp_p2p_->start_request_video_data(vid, clarity, index, is_mytv, pnum);
	}

	return -1;
}

void LocalHttpServer::stop_request_video_data(sh_int_64 unique_id)
{
	if (m_sp_p2p_.get())
	{
		m_sp_p2p_->stop_request_video_data(unique_id);
		download_info_.erase(unique_id); //删除任务队列里的uid
	}
}

void LocalHttpServer::pause_request_video_data(sh_int_64 unique_id)
{
	if (m_sp_p2p_.get())
	{
		m_sp_p2p_->pause_request_video_data(unique_id);
	}
}

void LocalHttpServer::restart_request_video_data(sh_int_64 unique_id)
{
	if (m_sp_p2p_.get())
	{
		m_sp_p2p_->restart_request_video_data(unique_id);
	}
}

sh_int_64 LocalHttpServer::start_play_video_data_range(sh_int_32 vid, enum SHVideoClarity clarity, bool is_mytv, sh_int_32 start_range, sh_int_32 end_range, sh_int_64 call_id)
{
	if (m_sp_p2p_.get())
	{
		//限制内存参数复位
		request_index_ = -1;
		section_completed_ = false;
		recv_counter_ = 0;
		is_find_index_ = false;

		if (start_range >= 0)
		{
			downloaded_len_ = start_range;
		}
		
		return m_sp_p2p_->start_request_video_data_range_ott(vid, clarity, is_mytv, kSHRequest_Play, start_range, end_range, call_id);
	}

	return -1;
}

sh_int_64 LocalHttpServer::start_download_video_data_range(sh_int_32 vid, enum SHVideoClarity clarity, bool is_mytv, sh_int_32 start_range, sh_int_32 end_range, sh_int_64 call_id)
{
	if (!m_sp_p2p_.get())
	{
		return -1;
	}

	off_downloaded_len_ = 0;

	INFO_LOG(LOCAL_SERVER_LOG, _T("start_download_video_data_range, vid = %d, start_range=%d, call_id=%lld, end_range=%d\r\n"), vid, start_range, call_id, end_range);

	return m_sp_p2p_->start_request_video_data_range_ott(vid, clarity, is_mytv, kSHRequest_Download, start_range, end_range, call_id);

	//原来路由器下载逻辑
	/*sh_int_64 unique_id = ((sh_int_64)is_mytv) << 32 | ((sh_int_64) clarity) << 33 | ((sh_int_64) kSHRequest_Download) << 38 | vid;
	std::map<sh_int_64, DOWNLOADVIDEOINFO>::iterator it = download_info_.find(unique_id);
	if (it != download_info_.end())
	{
		return -1; //已经在下载列表,直接返回
	}

	DOWNLOADVIDEOINFO info;
	info.vid = vid;
	info.definition = (int32_t)clarity;
	info.ismytv = is_mytv;

	sh_int_64 uid = m_sp_p2p_->start_download_video_data(vid, clarity, index, is_mytv, start_range, end_range);
	download_info_.insert(std::make_pair(uid, info));
	return uid;*/
}

void LocalHttpServer::set_allow_share( bool allow_share )
{
    if (m_sp_p2p_.get() == NULL)
    {
        return;
    }

    m_sp_p2p_->set_allow_share(allow_share);
}

bool LocalHttpServer::get_download_info(sh_int_64 unique_id, NewSHDispInfo* sh_disp_info)
{
	if (m_sp_p2p_.get() == NULL)
	{
		return false;
	}

	NewSHDispInfo download_info;
	while (m_sp_p2p_->get_download_info(&download_info))
	{
		if (unique_id == download_info.uid)
		{
			*sh_disp_info = download_info;
			return true;
		}
	}

	return false;
}

sh_int_32 LocalHttpServer::get_video_total_lens(sh_int_64 uid)
{
	sh_int_32 total_len = -1;
	std::map<sh_int_64, sh_int_32>::iterator it = map_video_lens_.find(uid);
	if (it != map_video_lens_.end())
	{
		total_len = it->second;
	}

	return total_len;
}

void LocalHttpServer::on_get_register_id(sh_int_32 id, sh_char_p sohu_key)
{
	char szRegisterId[100] = {0};
	sprintf(szRegisterId, "%d", id);

	std::string strPath = get_config_file();
	if (strPath.empty())
	{
		return;
	}

	WritePrivateProfileString("SOHU_P2P", "RegisterId", szRegisterId, strPath.c_str());
}

bool LocalHttpServer::WritePrivateProfileString(const char* lpAppName, const char* lpKeyName, const char* lpString, const char* lpFileName)
{
	char szSection[100] = {0};
	char szKey[100] = {0};
	char szTemp[260] = {0};
	sprintf(szSection, "[%s]", lpAppName);
	sprintf(szKey , "%s=" , lpKeyName);

 	if (access(lpFileName, 0) == -1)
 	{
		FILE* pfile = fopen(lpFileName, "w");
		if (pfile == NULL)
		{
			return false;
		}
		sprintf(szTemp, "%s\n%s%s\n", szSection, szKey, lpString);
		fwrite(szTemp, sizeof(char), strlen(szTemp), pfile);
		fclose(pfile);
		return true;
 	}

	std::string strConn = "";
	std::string strRow = "";
	size_t nSetionPos = std::string::npos;
	size_t nKeyPos = 0;
	FILE* pfile = NULL;
	pfile = fopen(lpFileName, "r");

	while (!feof(pfile))
	{
		memset(szTemp, 0, sizeof(szTemp));
		fscanf(pfile, "%s", szTemp);
		strRow = szTemp;
		if (nSetionPos == std::string::npos && (nSetionPos = strRow.find(szSection)) == 0)
		{ 
			//先找段
			strConn += szSection;
			strConn += "\n";
			nSetionPos = strConn.length(); 
			nKeyPos = std::string::npos;
		}
		else if (nKeyPos == std::string::npos && (nKeyPos = strRow.find(szKey)) == 0)
		{   
			//再找键
			strConn += szKey;
			strConn += lpString;
			strConn += "\n";
		}
		else
		{
			strConn += strRow;
			strConn += "\n";
		}  
	}
	fclose(pfile);

	if (nSetionPos == std::string::npos && nKeyPos == 0)
	{
		//没有找到段和键
		memset(szTemp, 0, sizeof(szTemp));
		sprintf(szTemp, "%s\n%s%s\n", szSection, szKey, lpString);
		strConn += szTemp;
	}
	else if (nSetionPos != std::string::npos && nKeyPos == std::string::npos)
	{
		//找到段了,但没有找到键值
		std::string strBack = strConn.substr(nSetionPos);
		strConn = strConn.substr(0, nSetionPos);
		strConn += szKey;
		strConn += lpString;
		strConn += "\n" + strBack;
	}
	else if (nSetionPos != std::string::npos && nKeyPos != std::string::npos)
	{
		//找到段了,也找到键值了
		memset(szTemp, 0, sizeof(szTemp));
		sprintf(szTemp, "%s", szKey);

		size_t pos = strConn.find(szTemp);
		std::string strFront, strBack, strText;
		if (pos == std::string::npos)
		{
			return false;
		}

		strFront = strConn.substr(0, pos);
		strText = strConn.substr(pos);

		pos = strText.find('\n');
		if (pos == std::string::npos)
		{
			return false;
		}
		strBack = strText.substr(pos);

		memset(szTemp, 0, sizeof(szTemp));
		sprintf(szTemp, "%s%s", szKey, lpString);
		strConn = strFront + szTemp + strBack;
	}

	pfile = fopen(lpFileName, "w"); 
	fwrite(strConn.c_str(), sizeof(char), strConn.length(), pfile);
	fclose(pfile);
	return true;
}

sh_int_32 LocalHttpServer::GetProfileInt( const char* lpAppName, const char* lpKeyName, const char* lpFileName )
{
	sh_int_32 nResult = 0;
	if (access(lpFileName, 0) == -1)
	{
		return nResult;
	}

	char szTemp[260] = {0};
	std::string strText = "";
	FILE* pfile = fopen(lpFileName, "r");
	if (pfile == NULL)
	{
		return nResult;
	}

	while (!feof(pfile))
	{
		memset(szTemp, 0, sizeof(szTemp));
		fscanf(pfile, "%s", szTemp);
		strText += szTemp;
		strText += "\n";
	}
	fclose(pfile);

	char szSection[100] = {0};
	char szKey[100] = {0};
	sprintf(szSection, "[%s]", lpAppName);
	sprintf(szKey , "%s=" , lpKeyName);

	//找对应段下面的数据
	size_t pos = strText.find(szSection);
	if (pos == std::string::npos)
	{
		return nResult;
	}

	std::string strSection = strText.substr(pos + strlen(szSection)+1);
	pos = strSection.find("[");
	if (pos != std::string::npos)
	{
		strSection = strSection.substr(0, pos); //找到段了
	}

	//找key
	pos = strSection.find(szKey);
	if (pos == std::string::npos)
	{
		return nResult;
	}

	std::string strKey = strSection.substr(pos);	
	pos = strKey.find('\n');
	if (pos == std::string::npos)
	{
		return nResult;
	}
	strKey = strKey.substr(0, pos); //找到key了

	//找key对应的值
	pos = strKey.find(szKey);
	if (pos == std::string::npos)
	{
		return nResult;
	}

	std::string strValue = strKey.substr(pos+strlen(szKey)); //找到值了
	nResult = atoi(strValue.c_str());
	return nResult;
}

std::string LocalHttpServer::get_config_file(void)
{
	std::string log_path;
	std::tstring str_path = get_log_path();
	if (str_path.empty())
	{
		return "";
	}

#ifdef WIN32
	if (str_path[str_path.size()-1] != _T('\\'))
	{
		str_path += _T("\\");
	}	
#else
	if (str_path[str_path.size()-1] != '/')
	{
		str_path += "/";
	}	
#endif

	//不存在则创建目录
	try
	{
		if(!SH_filesystem::dir_exist(str_path))
		{
			if (!SH_filesystem::create_dir(str_path))
			{
				return "";
			}	
		}
	}
	catch (...)
	{
		return "";
	}

	str_path += _T("config.ini");
	log_path = w2b(str_path);
	return log_path;
}

sh_int_64 LocalHttpServer::get_call_id_num(void)
{
	boost::mutex::scoped_lock lock(call_id_mutex_);
	static sh_int_64 call_id = 0;
	if (++call_id <= 0)
	{
		call_id = 1;
	}
	return call_id;
}

