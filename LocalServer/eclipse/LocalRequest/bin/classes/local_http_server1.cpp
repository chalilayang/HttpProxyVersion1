﻿#include "local_http_server.h"
#include "../p2pcommon/log/log.h"
#include "../include/p2p_export.h"
#include "../p2pcommon/base/file.h"
#include "../p2pcommon/base/algorithm.h"
#include "../include/sh_p2p_system_api.h"

boost::shared_ptr<LocalHttpServer> LocalHttpServer::ms_pinst;
boost::once_flag LocalHttpServer::ms_once_flag = BOOST_ONCE_INIT;
boost::shared_ptr<IOServicePool> LocalHttpServer::ios_pool_;

LocalHttpServer::LocalHttpServer() : is_running_(false)
{
	m_spP2pInterface.reset(get_p2p_instance());
	if (m_spP2pInterface.get())
	{
		m_spP2pInterface->advice(this);
	}

	memset(&p2p_notify_, 0, sizeof(p2p_notify_));
}

LocalHttpServer::~LocalHttpServer()
{

}

bool LocalHttpServer::init(unsigned int local_tcp_port)
{
    if (is_running_) return false;

	boost::asio::ip::address addr = boost::asio::ip::address::from_string("127.0.0.1");
    tcp::endpoint ep(addr,local_tcp_port);

	if (!ios_pool_)
	{
		ios_pool_.reset(new IOServicePool());
	}
	ios_pool_->start();

    http_server_ = HttpServer::create(ios_pool_->get_ios(0), ep, local_tcp_port, shared_from_this());

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
	if (m_spP2pInterface.get())
	{
		m_spP2pInterface->init_p2p_system(system_param);
	}

	register_log_file(system_param.log_path);
	
	init_p2p_system(system_param, p2p_notify_);

	return init(8834);
}

void LocalHttpServer::stop(void)
{
	if (m_spP2pInterface.get())
	{
		m_spP2pInterface->uninit_p2p_system();
	}

	uninit();
}

void LocalHttpServer::uninit(void)
{
    if (false == is_running_) return;

    if (http_server_)
    {
        http_server_->stop();
        http_server_.reset();
    }

   /* SHConnectionMap::iterator it;
    for (it = sock_connection_map_.begin(); it != sock_connection_map_.end();)
    {
		if (it->second.first)
		{
			it->second.first->Close();
		}
		it = sock_connection_map_.erase(it);
    }*/

	ios_pool_->stop();
	ios_pool_.reset();
	sock_connection_map_.clear();
    is_running_ = false;
}

void LocalHttpServer::on_accept(const boost::system::error_code& ec, boost::shared_ptr<tcp::socket> p_socket)
{
    if (false == is_running_) return;

	if (sock_connection_map_.find(p_socket) != sock_connection_map_.end())
	{
		INFO_LOG(LOCAL_SERVER_LOG,_T("socket map has this socket 0x%08x\n"), p_socket.get());
		sock_connection_map_.erase(p_socket);
	}

    boost::shared_ptr<LocalHttpConnection> connection = LocalHttpConnection::Create(shared_from_this());
    connection->Open(p_socket);
	sock_connection_map_.insert(std::make_pair(p_socket, std::make_pair(connection, -1)));
}

void LocalHttpServer::on_connection_close(boost::shared_ptr<LocalHttpConnection> con)
{
    if (false == is_running_) return;

    SHConnectionMap::iterator it;
	for (it = sock_connection_map_.begin(); it != sock_connection_map_.end();it++)
	{
		if (it->second.first == con)
			break;
	}

	if (it != sock_connection_map_.end())
	{
		sock_connection_map_.erase(it);
	}    
}

void LocalHttpServer::on_connection_start(sh_int_64 uid, boost::shared_ptr<LocalHttpConnection> sp_connection)
{
	if (false == is_running_) return;

	SHConnectionMap::iterator it;

	//如果有相同的uid,要先关闭掉原来的connection
	for (it = sock_connection_map_.begin(); it != sock_connection_map_.end();it++)
	{
		if (it->second.second == uid)
		{
			if (it->second.first.get() != NULL)
			{
				it->second.first->Close();
				INFO_LOG(LOCAL_SERVER_LOG, _T("删除已有的 connection,  uid = %lld\r\n"), uid);
				break;
			}
		}
	}

	//保存正在下载的uid
	for (it = sock_connection_map_.begin(); it != sock_connection_map_.end();it++)
	{
		if (it->second.first == sp_connection)
		{
			it->second.second = uid;
			break;
		}
	}
}

void LocalHttpServer::on_p2p_happen_error(sh_int_64 unique_id, sh_uint_32 index, enum SHP2pErrorType error_type, sh_uint_32 error_code, sh_uint_32 status_code)
{
	if (false == is_running_) return;

	boost::shared_ptr<LocalHttpConnection> sp_connection = get_http_connection(unique_id);
	if (sp_connection.get() == NULL)
	{
		return;
	}

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

void LocalHttpServer::on_recv_p2p_video_data(sh_int_64 unique_id, sh_uint_32 index, sh_int_8_p data, sh_uint_32 len, bool isheader, sh_int_32 real_mp4_size)
{
	if (false == is_running_) return;

	boost::shared_ptr<LocalHttpConnection> sp_connection = get_http_connection(unique_id);
	if (sp_connection.get() == NULL)
	{
		INFO_LOG(LOCAL_SERVER_LOG, _T("connection is NULL,  uid = %lld, section = %d\r\n"), unique_id, index);
		return;
	}

	if (ios_pool_)
	{
		ios_pool_->get_ios(0).post(boost::bind(&LocalHttpServer::send_data, LocalHttpServer::Inst(), sp_connection, string(data, len), len, isheader, real_mp4_size));
	}
}

void LocalHttpServer::on_finish_p2p_video_data(sh_int_64 unique_id)
{
	if (false == is_running_) return;

	SHConnectionMap::iterator it;
	for (it = sock_connection_map_.begin(); it != sock_connection_map_.end(); it++)
	{
		if (it->second.second == unique_id)
		{
			break;
		}
	}

	if (it != sock_connection_map_.end())
	{
		sock_connection_map_.erase(it);
	}    
}

boost::shared_ptr<LocalHttpConnection> LocalHttpServer::get_http_connection(sh_int_64 unique_id)
{
	boost::shared_ptr<LocalHttpConnection> sp_connection;
	SHConnectionMap::iterator it;
	for (it = sock_connection_map_.begin(); it != sock_connection_map_.end(); it++)
	{
		if (it->second.second == unique_id)
		{
			sp_connection = it->second.first;
			break;
		}
	}

	return sp_connection;
}

void LocalHttpServer::send_data(boost::shared_ptr<LocalHttpConnection> sp_conn, std::string &data, int len, bool isheader, sh_int_32 real_mp4_size)
{
	if (false == is_running_) return;

	if (sp_conn.get() != NULL)
	{
		sp_conn->send_data(data, len, isheader, real_mp4_size);
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
	if (m_spP2pInterface.get())
	{
		return m_spP2pInterface->start_request_video_data(vid, clarity, index, is_mytv, pnum);
	}

	return -1;
}
