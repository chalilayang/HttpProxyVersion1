#include "local_http_connection.h"
#include "local_http_server.h"
#include "../p2pcommon/log/log.h"
#include "../p2pcommon/base/algorithm.h"
#include "../p2pcommon/base/file.h"
#include "json/json.h"
#include "md5.h"

boost::shared_ptr<LocalHttpConnection> LocalHttpConnection::Create(boost::shared_ptr<LocalHttpServer> localserver)
{
    boost::shared_ptr<LocalHttpConnection> ret(new LocalHttpConnection(localserver));
    return ret;
}

LocalHttpConnection::LocalHttpConnection(boost::shared_ptr<LocalHttpServer> localserver)
: local_http_server_(localserver), is_open_(false), erro_count_(0)
{
    Init();
}

void LocalHttpConnection::Init(void)
{
	m_last_timeout_ = 0;
	download_type_ = SHDOWNLOAD_NONE;

#ifdef WRITE_FILE_POS
	m_file_ = NULL;
#endif
}

void LocalHttpConnection::Open(boost::shared_ptr<tcp::socket> psock)
{
	if (is_open_) return;

	if (psock)
	{
		http_service_ = HttpService::create(psock, shared_from_this());
		http_service_->start();
		is_open_ = true;
	}
}

void LocalHttpConnection::Close(void)
{
	if (false == is_open_) return;
	
	INFO_LOG(LOCAL_SERVER_LOG, _T("已调用 HTTP 关闭连接函数! this=0x%08x\r\n"), shared_from_this().get());

	is_open_ = false;

	if (http_service_)
	{
		http_service_->close();
		//http_service_.reset();
	}
}


LocalHttpConnection::~LocalHttpConnection()
{
	INFO_LOG(LOCAL_SERVER_LOG, _T("Connection 已析构! this=%08x\r\n"), this);

#ifdef WRITE_FILE_POS
	if (m_file_ != NULL)
	{
		fclose(m_file_);
	}
#endif
}

void LocalHttpConnection::on_service_close(void)
{
	if (!local_http_server_.expired())
	{
		local_http_server_.lock()->on_connection_close(shared_from_this(), request_info_.call_id);
	}
}

void LocalHttpConnection::on_service_read(const boost::system::error_code& ec, HttpRequest::HttpRequestPtr p_request, IOBuffer content_buf)
{
	if (!is_open_ || local_http_server_.expired())
	{
		return;
	}
	
    if (!ec && p_request)
	{
		std::string command = p_request->get_path();
		std::tstring strLog = b2w(command);
		INFO_LOG(LOCAL_SERVER_LOG, _T("解析 http 请求: %s\n"), strLog.c_str());

		if (command.find("/crossdomain.xml")!= string::npos)
		{
			std::string str_resp(FLASH_CROSS_DOMAIN_HEADER);
			response200_xml(str_resp);
		}
		else if (command.find("live.m3u8") != string::npos)
		{
			INFO_LOG(LOCAL_SERVER_LOG, _T("收到狐播请求: %s\n"), strLog.c_str());
			process_foxplay_request(command);
		}
		else if (parse_http_request(command))
		{
			int64_t begin_pos = 0, end_pos = 0;
			if (p_request->get_range(begin_pos, end_pos) && request_info_.startpos == 0)
			{
				INFO_LOG(LOCAL_SERVER_LOG, _T("收到播放器 SEEK 请求! begin = %lld, end = %lld\n"), begin_pos, end_pos);
				request_info_.startpos = begin_pos;
			}

		  	process_http_request();
		}
	}
	else
	{
		INFO_LOG(LOCAL_SERVER_LOG, _T("读取HTTP请求数据发生错误! error: %d, %s, p_request = %d\n"), ec.value(), b2w(ec.message()).c_str(), p_request.get());
		
		Close();
	}
}

void LocalHttpConnection::on_service_read_timeout(void)
{
    if (false == is_open_) return;
}

void LocalHttpConnection::on_service_write(const boost::system::error_code& ec, size_t trans_bytes)
{
	if (false == is_open_) return;

	if (!ec)
	{
		erro_count_ = 0;
	}
	else
	{
		erro_count_++;
		//if (erro_count_ > 3 || ec.value() == 32 || ec.value() == 10053 || ec == boost::asio::error::operation_aborted || ec == boost::asio::error::eof||ec == boost::asio::error::connection_reset)
		{
			if (download_type_ == SHDOWNLOAD_OFFLINE && !local_http_server_.expired())
			{
				local_http_server_.lock()->on_stop_request_video_data(shared_from_this());
				INFO_LOG(LOCAL_SERVER_LOG, _T("停止P2P 离线下载! %d, %s\n"), ec.value(), b2w(ec.message()).c_str());
			}
			else if (request_info_.isclose && !local_http_server_.expired())
			{
				local_http_server_.lock()->on_stop_request_video_data(shared_from_this());
			}

			Close();

			if (get_tickcount() - m_last_timeout_ > 2000)
			{
				m_last_timeout_ = get_tickcount();				
				erro_count_ = 0;
				//INFO_LOG(LOCAL_SERVER_LOG, _T("超时,connection已关闭: %d, %s\n"), ec.value(), b2w(ec.message()).c_str());
			}
		}
	}
}

void LocalHttpConnection::on_service_request_video_data(int buffer_size)
{
	if (local_http_server_.expired())
	{
		return;
	}

	if (download_type_ == SHDOWNLOAD_PLAY && local_http_server_.lock()->is_request_section_completed() && buffer_size < LOCAL_SERVER_BUFFER_SIZE)
	{
		return;
		int pos = local_http_server_.lock()->get_downloaded_pos();
		sh_int_64 uid = local_http_server_.lock()->start_play_video_data_range(request_info_.vid,(SHVideoClarity)request_info_.definition,request_info_.isMy,pos,-1,request_info_.call_id);
		INFO_LOG(LOCAL_SERVER_LOG, _T("控制内存大小,内部自动 RANGE 请求视频下一段mp4数据, start = %d, buffer_size=%d\r\n"), pos, buffer_size);
	}
}

void LocalHttpConnection::response_200_ok_plain(std::string &str_msg)
{
	HttpResponse::HttpResponsePtr http_response(new HttpResponse("HTTP/1.1",HttpResponse::SH_HTTP_STATUS_OK));
	http_response->set_header("Content-Type", "text/plain");
	http_response->set_header("Content-Length", boost::lexical_cast<std::string>(str_msg.size()));
	http_response->set_header("Connection", "close");

	if (http_service_)
	{
		std::string str_content = http_response->serialize_to_string();
		if (!str_msg.empty())
		{
			str_content.append(str_msg);
		}

		IOBuffer str_response(str_content);
		http_service_->write(str_response);
	}	
}

void LocalHttpConnection::response200_xml(std::string &str_msg)
{
	HttpResponse::HttpResponsePtr http_response(new HttpResponse("HTTP/1.1",HttpResponse::SH_HTTP_STATUS_OK));
	http_response->set_header("Content-Type", "text/xml");
	http_response->set_header("Content-Length", boost::lexical_cast<std::string>(str_msg.size()));
	http_response->set_header("Connection", "close");

	if (http_service_ )
	{
		http_service_->write(http_response->serialize_to_buffer());
		if(!str_msg.empty())
		{
			IOBuffer msg(str_msg);
			http_service_->write(msg);
		}
	}
}

bool LocalHttpConnection::parse_http_request(std::string data)
{
	request_info_.init();
	size_t pos = 0;
	if(data.size() > 0 && data[0] == '/')
		data.erase(0,1);

	//type
	if((pos = data.find("?",0)) == string::npos)
		return false;
	request_info_.type = data.substr(0,pos);
	if (request_info_.type.empty())
	{
		return false;
	}

	data.erase(0,pos+1);
	std::vector<string> strs = SpliterString(data,"&");
	string userid("");
	for (size_t i = 0; i < strs.size(); ++i)
	{
		vector<string> attri = SpliterString(strs[i], "=");
		if (attri.size() != 2)
			continue;
		trim_str(attri[1]);
		if (attri[0] == "dnum")
		{
			request_info_.dnum = atoi(attri[1].c_str());
		}
		else if (attri[0] == "pnum")
		{
			request_info_.pnum = atoi(attri[1].c_str());
		}
		else if (attri[0] == "vid")
		{
			request_info_.vid= atoi(attri[1].c_str());
		}
		else if (attri[0] == "startpos")
		{
			request_info_.startpos= atoi(attri[1].c_str());
		}
		else if (attri[0].compare("definition") == 0)
		{
			request_info_.definition = atoi(attri[1].c_str());
		}
		else if (attri[0].compare("ismytv") == 0)
		{
			request_info_.isMy = bool(atoi(attri[1].c_str()));
		}
		else if (attri[0].compare("uid") == 0)
		{
			request_info_.unique_id = _atoi64(attri[1].c_str());
		}
		else if (attri[0].compare("isclose") == 0)
		{
			request_info_.isclose = bool(atoi(attri[1].c_str()));;
		}
	}

	request_info_.call_id = LocalHttpServer::Inst()->get_call_id_num();

	if (request_info_.vid == 0)
		return false;

	return true;
}

void LocalHttpConnection::send_data(std::string &str_data, int len, bool isheader /*= false*/, sh_int_32 real_mp4_size /*= 0*/, sh_int_32 cur_download_pos)
{
	if (!is_open_ || local_http_server_.expired() || http_service_.get() == NULL)
	{
		return;
	}

	if (isheader && len > 0)
	{
		ostringstream stream;
		stream<<"HTTP/1.1 200 OK\r\nServer: p2plocalserver\r\nContent-Type: video/mp4\r\n";
		stream<<"Content-Length: "<<real_mp4_size<<"\r\n";
		if (download_type_ != SHDOWNLOAD_PLAY)
			stream<<"Mp4Header-Length: "<<len<<"\r\n";
		stream<<"Accept-Ranges: bytes"<<"\r\n";
        stream<<"Connection: keep-alive\r\n\r\n";
		IOBuffer str_response(stream.str());
		http_service_->write(str_response);
		INFO_LOG(LOCAL_SERVER_LOG, _T("下载或播放发送头数据: header_len=%d, real_mp4_size=%d\r\n"), len, real_mp4_size);
	}
	else if (isheader && len <= 0) 
	{
		sh_int_32 send_len = real_mp4_size-request_info_.startpos;
		ostringstream stream;
		stream<<"HTTP/1.1 206 Partial Content\r\nServer: p2plocalserver\r\nContent-Type: video/mp4\r\n";
		stream<<"Content-Length: "<<send_len<<"\r\n";
		stream<<"Mp4Header-Length: "<<len<<"\r\n";
		stream<<"Content-Range: bytes "<<request_info_.startpos<<"-"<<real_mp4_size-1<<"/"<<real_mp4_size<<"\r\n";
		stream<<"Connection: keep-alive\r\n\r\n";
		IOBuffer str_response(stream.str());
		http_service_->write(str_response);
		INFO_LOG(LOCAL_SERVER_LOG, _T("断点下载发送头数据: startpos=%d, header_len=%d, real_mp4_size=%d\r\n"), request_info_.startpos, len, real_mp4_size);
		return;		
	}

	if (request_info_.startpos > 0 && cur_download_pos == 0)
	{
		//下一段数据
	}
	else if (request_info_.startpos > 0 && cur_download_pos > 0 && request_info_.startpos != cur_download_pos)
	{
		return;
	}

	IOBuffer str_response(str_data);
	http_service_->write(str_response);

#ifdef WRITE_FILE_POS
	//保存播放seek时的数据，只为测试使用
	if (m_file_ != NULL && download_type_ == SHDOWNLOAD_PLAY && !isheader && len > 0 && request_info_.startpos > 0 && cur_download_pos > 0)
	{
		fwrite(str_data.data(), str_data.size(), 1, m_file_);
		fflush(m_file_);
	}
#endif
}


void LocalHttpConnection::process_http_request(void)
{
	if (local_http_server_.expired())
	{
		return;
	}

	if (request_info_.type.compare("start_play") == 0)
	{
#if 1
		download_type_ = SHDOWNLOAD_PLAY;

		//先关闭原来正在播放的connection 上层和底层都是用这个作为唯一标示
		sh_int_64 unique_id = ((sh_int_64)request_info_.isMy) << 32 | ((sh_int_64) request_info_.definition) << 33 | ((sh_int_64) kSHRequest_Play) << 38 | request_info_.vid;
		//local_http_server_.lock()->on_connection_close(unique_id, shared_from_this());

		//Seek 
		if (request_info_.startpos > 0)
		{
			send_seek_header();

#ifdef WRITE_FILE_POS
			std::string file_path = get_log_path();
			m_file_ = fopen(file_path.c_str(), "wb");
			INFO_LOG(LOCAL_SERVER_LOG, _T("拖拽时保存文件, file= %s, startpos = %d\r\n"), b2w(file_path).c_str(), request_info_.startpos);
#endif
		}

		//播放
		local_http_server_.lock()->on_connection_update(unique_id, request_info_.call_id, shared_from_this());
		sh_int_64 uid = local_http_server_.lock()->start_play_video_data_range(request_info_.vid,(SHVideoClarity)request_info_.definition,request_info_.isMy,request_info_.startpos,-1, request_info_.call_id);
		m_last_timeout_ = get_tickcount();
		INFO_LOG(LOCAL_SERVER_LOG, _T("开始视频播放, startpos = %d, call_id=%d, vid=%d, uid=%lld, ismytv=%d, definition=%d\r\n"), request_info_.startpos, request_info_.call_id, request_info_.vid, uid, request_info_.isMy, request_info_.definition);
#endif

#if 0
		std::string log_path;
		std::tstring str_path = LocalHttpServer::Inst()->get_log_path();

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

		//本地播放测试
		//FILE *pfile = fopen("D:\\test\\ott\\movie.mp4", "rb");
		INFO_LOG(LOCAL_SERVER_LOG, _T("打开本地文件=%s, startpos = %d\r\n"), str_path.c_str(), request_info_.startpos);
		FILE *pfile = fopen(w2b(str_path).c_str(), "rb");
		if (pfile == NULL)
		{
			return;
		}
		fseek(pfile, 0, SEEK_END);
		int total_len = ftell(pfile);
		fseek(pfile, 0, SEEK_SET);

		char data[16*1024] = {0};
		int nReadLen = 0;

		std::string data_buf;

		//先给mp4头 头总大小1044525
		if (request_info_.startpos != 0)
		{
			char *pdata = new char[1044525];
			nReadLen = fread(pdata, 1, 1044525, pfile);

			//文件指针移动到request_info_.startpos
			fseek(pfile, request_info_.startpos, SEEK_SET);

			//发送HTTP头
			sh_int_32 len = total_len-request_info_.startpos;
			ostringstream stream;
			stream<<"HTTP/1.1 206 Partial Content\r\nServer: p2plocalserver\r\nContent-Type: video/mp4\r\n";
			stream<<"Content-Length: "<<len<<"\r\n";
			stream<<"Content-Range: bytes "<<request_info_.startpos<<"-"<<total_len-1<<"/"<<total_len<<"\r\n";
			stream<<"Connection: close\r\n\r\n";
			IOBuffer str_response(stream.str());
			http_service_->write(str_response);
			int nsize = str_response.size();
		}
		else 
		{
			nReadLen = fread(data, 1, sizeof(data), pfile);
			data_buf= std::string(data, nReadLen);
			send_data(data_buf, nReadLen, true, total_len, -2);
			total_len -= nReadLen;
		}

		do 
		{
			nReadLen = fread(data, 1, sizeof(data), pfile);
			data_buf = std::string(data, nReadLen);
			send_data(data_buf, nReadLen, false, total_len, -2);
			total_len -= nReadLen;

//#ifdef WIN32
//			Sleep(10);
//#else
//			usleep(10 * 1000);
//#endif
			
		} while (nReadLen > 0);

		fclose(pfile);
#endif
	}
	else if (request_info_.type.compare("stop_play") == 0)
	{
		//停止播放
		sh_int_64 unique_id = ((sh_int_64)request_info_.isMy) << 32 | ((sh_int_64) request_info_.definition) << 33 | ((sh_int_64) kSHRequest_Play) << 38 | request_info_.vid;
		local_http_server_.lock()->stop_request_video_data(unique_id);
		INFO_LOG(LOCAL_SERVER_LOG, _T("停止视频播放, startpos = %d, vid=%d, uid=%lld, ismytv=%d, definition=%d\r\n"), request_info_.startpos, request_info_.vid, unique_id, request_info_.isMy, request_info_.definition);
		std::string response("ok");
		response_200_ok_plain(response);
		local_http_server_.lock()->on_connection_update(unique_id, request_info_.call_id, shared_from_this());
		Close();
	}
	else if (request_info_.type.compare("download") == 0)
	{
		download_type_ = SHDOWNLOAD_OFFLINE;
		
		//先关闭掉原来正在下载的connection
		//sh_int_64 unique_id = ((sh_int_64)request_info_.isMy) << 32 | ((sh_int_64) request_info_.definition) << 33 | ((sh_int_64) kSHRequest_Download) << 38 | request_info_.vid;
		//local_http_server_.lock()->on_connection_close(unique_id);
		
		//创建离线下载任务 类型 kSHRequest_Download
		sh_int_64 uid = local_http_server_.lock()->start_download_video_data_range(request_info_.vid,(SHVideoClarity)request_info_.definition,request_info_.isMy,request_info_.startpos,-1,request_info_.call_id);
		INFO_LOG(LOCAL_SERVER_LOG, _T("创建离线下载任务, vid = %d, uid = %lld, start_pos=%d, call_id=%d\r\n"), request_info_.vid, uid, request_info_.startpos, request_info_.call_id);
		local_http_server_.lock()->on_connection_update(uid, request_info_.call_id, shared_from_this());
	}
	else if (request_info_.type.compare("shakehand") == 0)
	{
		std::string str_data;
		ostringstream os;
		sh_int_64 unique_id = ((sh_int_64)request_info_.isMy) << 32 | ((sh_int_64) request_info_.definition) << 33 | ((sh_int_64) kSHRequest_Play) << 38 | request_info_.vid;

		os << "vid=" << request_info_.vid  << "|" << "ismytv=" << request_info_.isMy  << "|" << "definition=" << request_info_.definition;
		str_data = os.str();
		response_200_ok_plain(str_data);
		local_http_server_.lock()->on_connection_update(unique_id, request_info_.call_id, shared_from_this());
		Close();
		INFO_LOG(LOCAL_SERVER_LOG, _T("握手, vid=%d, ismytv=%d, definition=%d\r\n"), request_info_.vid, request_info_.isMy, request_info_.definition);
	}
	else if (request_info_.type.compare("get_param") == 0)
	{
		INFO_LOG(LOCAL_SERVER_LOG, _T("获取p2p上报参数, vid = %d, uid = %lld\r\n"), request_info_.vid, request_info_.unique_id);
		response_get_p2p_reported_param();
		sh_int_64 unique_id = ((sh_int_64)request_info_.isMy) << 32 | ((sh_int_64) request_info_.definition) << 33 | ((sh_int_64) kSHRequest_Play) << 38 | request_info_.vid;
		local_http_server_.lock()->on_connection_update(unique_id, request_info_.call_id, shared_from_this());
		Close();
	}
	else if (request_info_.type.compare("get_offline_download_info") == 0)
	{
		//获取离线下载信息
		INFO_LOG(LOCAL_SERVER_LOG, _T("获取离线下载信息, vid = %d, uid = %lld\r\n"), request_info_.vid, request_info_.unique_id);
		response_get_offline_download_info();
		sh_int_64 unique_id = ((sh_int_64)request_info_.isMy) << 32 | ((sh_int_64) request_info_.definition) << 33 | ((sh_int_64) kSHRequest_Play) << 38 | request_info_.vid;
		local_http_server_.lock()->on_connection_update(unique_id, request_info_.call_id, shared_from_this());
		Close();
	}
	else if (request_info_.type.compare("pause_task_download") == 0)
	{
		//暂停任务下载
		INFO_LOG(LOCAL_SERVER_LOG, _T("暂停任务下载, vid = %d, uid = %lld\r\n"), request_info_.vid, request_info_.unique_id);
		response_pause_download_task();
		sh_int_64 unique_id = ((sh_int_64)request_info_.isMy) << 32 | ((sh_int_64) request_info_.definition) << 33 | ((sh_int_64) kSHRequest_Play) << 38 | request_info_.vid;
		local_http_server_.lock()->on_connection_update(unique_id, request_info_.call_id, shared_from_this());
		Close();
	}
	else if (request_info_.type.compare("restart_task_download") == 0)
	{
		//开始任务下载
		INFO_LOG(LOCAL_SERVER_LOG, _T("开始任务下载, vid = %d, uid = %lld\r\n"), request_info_.vid, request_info_.unique_id);
		response_restart_download_task();
		sh_int_64 unique_id = ((sh_int_64)request_info_.isMy) << 32 | ((sh_int_64) request_info_.definition) << 33 | ((sh_int_64) kSHRequest_Play) << 38 | request_info_.vid;
		local_http_server_.lock()->on_connection_update(unique_id, request_info_.call_id, shared_from_this());
		Close();
	}
	else if (request_info_.type.compare("delete_task_download") == 0)
	{
		//删除任务下载
		INFO_LOG(LOCAL_SERVER_LOG, _T("删除任务下载, vid = %d, uid = %lld\r\n"), request_info_.vid, request_info_.unique_id);
		response_delete_download_task();
		sh_int_64 unique_id = ((sh_int_64)request_info_.isMy) << 32 | ((sh_int_64) request_info_.definition) << 33 | ((sh_int_64) kSHRequest_Play) << 38 | request_info_.vid;
		local_http_server_.lock()->on_connection_update(unique_id, request_info_.call_id, shared_from_this());
		Close();
	}
	else 
	{
		INFO_LOG(LOCAL_SERVER_LOG, _T("HTTP 请求默认处理!\r\n"));
		std::string response("");
		response_200_ok_plain(response);
		sh_int_64 unique_id = ((sh_int_64)request_info_.isMy) << 32 | ((sh_int_64) request_info_.definition) << 33 | ((sh_int_64) kSHRequest_Play) << 38 | request_info_.vid;
		local_http_server_.lock()->on_connection_update(unique_id, request_info_.call_id, shared_from_this());
		Close();
	}
}

bool LocalHttpConnection::response_get_p2p_reported_param(void)
{
	if (local_http_server_.expired())
	{
		std::string str_resp("");
		response_200_ok_plain(str_resp);
		INFO_LOG(LOCAL_SERVER_LOG, _T("获取p2p参数失败!\r\n"));
		return false;
	}

	Json::FastWriter writer;
	Json::Value out;
#ifdef WIN32
	out["isp2p"] = Json::Value(0);
#else
	out["isp2p"] = Json::Value(0);
#endif
	out["cdnid"] = Json::Value(3469);
	out["cdnip"] = Json::Value("10.2.11.56");
	out["clientip"] = Json::Value("241.45.37.32");
	out["dufile"] = Json::Value("http://123.125.123.80/p2p?new=/244/59/f3WSezYdSPqNPePWNOIU4B.mp4&num=3&key=aFA0Rhy6SZwQhMRvpM6Uk2phTmmiZoGC");
	out["clientip"] = Json::Value("http://123.126.104.11/sohu/s26h23eab6/p2p/TGwyTGPAqK2m0JVlomXEs9bss5qxtSixskY9ELVhkE-KTlmyqr/f3WSezYd?key=nTdTNUyFQv5mBqkofhm1xDvfTEMjfVv4EfFisw..&ch=tv&catcode=101103;101122&idc=378&pt=0&pg=1&prod=ifox&rs=1");
	out["httpcode"] = Json::Value(200);

	std::string str_response = writer.write(out);
	response_200_ok_plain(str_response);

	INFO_LOG(LOCAL_SERVER_LOG, _T("返回p2p参数, content=%s\r\n"), str_response.c_str());

}

bool LocalHttpConnection::response_get_offline_download_info(void)
{
	if (local_http_server_.expired())
	{
		return false;
	}

	//uid 视频唯一标识
	sh_int_64 unique_id = ((sh_int_64)request_info_.isMy) << 32 | ((sh_int_64) request_info_.definition) << 33 | ((sh_int_64) kSHRequest_Download) << 38 | request_info_.vid;

	NewSHDispInfo download_info;
	memset(&download_info, 0, sizeof(download_info));
 	if (!local_http_server_.lock()->get_download_info(unique_id, &download_info))
 	{
 		std::string str_resp("");
 		response_200_ok_plain(str_resp);
 		INFO_LOG(LOCAL_SERVER_LOG, _T("离线下载信息不存在\r\n"));
 		return false;
 	}

	Json::FastWriter writer;
	Json::Value out;
#ifdef WIN32
	out["name"] = Json::Value(w2b(download_info.file_name).c_str());
#else
	out["name"] = Json::Value(download_info.file_name);
#endif
	out["speed"] = Json::Value(download_info.average_speed);
	out["percent"] = Json::Value(download_info.percent);
	out["state"] = Json::Value(download_info.state);

	std::string str_response = writer.write(out);
	response_200_ok_plain(str_response);

	INFO_LOG(LOCAL_SERVER_LOG, _T("返回离线下载信息, content=%s\r\n"), str_response.c_str());

	return true;
}

bool LocalHttpConnection::response_pause_download_task(void)
{
	if (local_http_server_.expired())
	{
		return false;
	}

	//uid 视频唯一标识
	sh_int_64 unique_id = ((sh_int_64)request_info_.isMy) << 32 | ((sh_int_64) request_info_.definition) << 33 | ((sh_int_64) kSHRequest_Download) << 38 | request_info_.vid;
	local_http_server_.lock()->pause_request_video_data(unique_id);

	//回复响应
	std::string str_resp("ok");
	response_200_ok_plain(str_resp);
	return true;
}

bool LocalHttpConnection::response_restart_download_task(void)
{
	if (local_http_server_.expired())
	{
		return false;
	}

	//uid 视频唯一标识
	sh_int_64 unique_id = ((sh_int_64)request_info_.isMy) << 32 | ((sh_int_64) request_info_.definition) << 33 | ((sh_int_64) kSHRequest_Download) << 38 | request_info_.vid;
	local_http_server_.lock()->restart_request_video_data(unique_id);

	//回复响应
	std::string str_resp("ok");
	response_200_ok_plain(str_resp);
	return true;
}

bool LocalHttpConnection::response_delete_download_task(void)
{
	if (local_http_server_.expired())
	{
		return false;
	}

	//uid 视频唯一标识
	sh_int_64 unique_id = ((sh_int_64)request_info_.isMy) << 32 | ((sh_int_64) request_info_.definition) << 33 | ((sh_int_64) kSHRequest_Download) << 38 | request_info_.vid;
	local_http_server_.lock()->stop_request_video_data(unique_id);

	//回复响应
	std::string str_resp("ok");
	response_200_ok_plain(str_resp);
	return true;
}

void LocalHttpConnection::send_seek_header(void)
{
	if (local_http_server_.expired())
	{
		return;
	}

	sh_int_64 unique_id = ((sh_int_64)request_info_.isMy) << 32 | ((sh_int_64) request_info_.definition) << 33 | ((sh_int_64) kSHRequest_Play) << 38 | request_info_.vid;
	sh_int_32 total_len = local_http_server_.lock()->get_video_total_lens(unique_id);

	//发送HTTP头
	sh_int_32 len = total_len-request_info_.startpos;
	ostringstream stream;
	stream<<"HTTP/1.1 206 Partial Content\r\nServer: p2plocalserver\r\nContent-Type: video/mp4\r\n";
	stream<<"Content-Length: "<<len<<"\r\n";
	stream<<"Content-Range: bytes "<<request_info_.startpos<<"-"<<total_len-1<<"/"<<total_len<<"\r\n";
	stream<<"Connection: keep-alive\r\n\r\n";
	IOBuffer str_response(stream.str());
	http_service_->write(str_response);
	int nsize = str_response.size();

	INFO_LOG(LOCAL_SERVER_LOG, _T("收到 SEEK 请求时, 发送 HTTP 头信息! startpos=%d, size=%d, total_len=%d\r\n"), request_info_.startpos, str_response.size(), total_len);
}

bool LocalHttpConnection::is_buffer_empty(void)
{
	if (http_service_)
	{
		return http_service_->is_queue_empty();
	}

	return false;
}

void LocalHttpConnection::process_foxplay_request(std::string data) {
	download_type_ = SHDOWNLOAD_FOXPLAY;
	size_t pos = 0;
	if (data.size() > 0 && data[0] == '/')
	{
		data.erase(0, 1);
	}

	size_t pos_tmp = data.find("?");
	if (pos_tmp != std::string::npos && pos_tmp >= 0) {
		std::string tmp_str = data.substr(pos_tmp+1);
		vector<string> strs;
		Splite(strs, tmp_str, "&");
		if (strs.size() <= 0) {
			return;
		}
		std::map<std::string, std::string> str_map;
		for (int index = 0, count = strs.size(); index < count; index ++) {
			std::string str_ = strs[index];
			size_t colon_pos = str_.find_first_of("=");
			if( colon_pos == std::string::npos ) continue;

			std::string key = boost::algorithm::trim_copy(str_.substr(0, colon_pos));
			std::string val = boost::algorithm::trim_copy(str_.substr(colon_pos+1));

			str_map.insert(std::pair<std::string, std::string>(key, val) );
		}
		std::ostringstream packetStream;
		packetStream << getSystemTime();
		std::string ts_str = packetStream.str();
		str_map.insert(std::pair<std::string, std::string>("ts", ts_str) );

		std::string params_to_verify;
		std::map<std::string, std::string >::iterator it = str_map.begin();
		while(it != str_map.end())
		{
		   if(it->first != "streamName" && it->first != "main")
		   {
			   params_to_verify.append(it->first).append("=").append(it->second);
		   }
		   it++;
		}
		std::string HOST_NAME;  //域名
		std::string FOX_PLAY_MD5_KEY;   //用于加密的key
		it = str_map.find("fox_play_api_key");
		if (it != str_map.end()) {
			FOX_PLAY_MD5_KEY = it->second;
		} else {
			return;
		}
		it = str_map.find("host_name");
		if (it != str_map.end()) {
			HOST_NAME = it->second;
		} else {
			return;
		}
		params_to_verify.append(FOX_PLAY_MD5_KEY);
		INFO_LOG(LOCAL_SERVER_LOG, _T("params_to_verify: %s\n"), params_to_verify.c_str());

		char *cstr = new char[params_to_verify.length() + 1];
	  strcpy(cstr,params_to_verify.c_str());

	  char *md5_verify_char = new char[33];

	  MD5_hash(md5_verify_char, cstr);

	  std::string verify_str("verify=");
	  verify_str.append(md5_verify_char);

	  delete[] cstr;
	  delete[] md5_verify_char;

		std::string result_url;
		std::string streamName_str = str_map.find("streamName")->second;
		std::string main_str = str_map.find("main")->second;
		std::string api_key_str = str_map.find("api_key")->second;
		std::string poid_str = str_map.find("poid")->second;
		std::string plat_str = str_map.find("plat")->second;
		std::string sver_str = str_map.find("sver")->second;
		std::string partner_str = str_map.find("partner")->second;
		std::string client_str = str_map.find("client")->second;
		std::string uid_str = str_map.find("uid")->second;

		result_url.append("http://").append(HOST_NAME).append("/live.m3u8?")
		.append("streamName").append("=").append(streamName_str).append("&")
		.append("main").append("=").append(main_str).append("&")
		.append("api_key").append("=").append(api_key_str).append("&")
		.append("poid").append("=").append(poid_str).append("&")
		.append("plat").append("=").append(plat_str).append("&")
		.append("sver").append("=").append(sver_str).append("&")
		.append("partner").append("=").append(partner_str).append("&")
		.append("client").append("=").append(client_str).append("&")
		.append("uid").append("=").append(uid_str).append("&")
		.append("ts").append("=").append(ts_str).append("&")
		.append(verify_str);

		INFO_LOG(LOCAL_SERVER_LOG, _T("加密完毕: %s\n"), result_url.c_str());

		HttpRequest::HttpRequestPtr prequest;
		prequest = HttpRequest::create_from_url(result_url);

		prequest->set_header("User-Agent", "sohuOTT foxplay");
		prequest->set_header("Connection", "Close");

		if (http_service_) {
			this->remote_host_handler_ptr = RemoteHostHandler::create(
					http_service_->get_socket()->get_io_service(),
					shared_from_this());
			this->remote_host_handler_ptr->start_get_remote_host_data(prequest);
		}
	}
//	std::string url =
//			"http://live.hubo.ott.sohu.com/live.m3u8?streamName=zonghe_c&main=true&api_key=7ad23396564b27116418d3c03a77db45&poid=12&plat=15&sver=3.1.0&partner=999&client=10&uid=2230774f04b2a2e5f26fe785f011a92c";

}

void LocalHttpConnection::on_remote_host_data_recieved(const IOBuffer& io_buf) {
	if (http_service_) {
		INFO_LOG(LOCAL_SERVER_LOG, _T("成功读取狐播服务器数据\r\n"),
				shared_from_this().get());
		http_service_->write(io_buf);
	}
}

std::string LocalHttpConnection::get_log_filename(void)
{

#ifdef WIN32
	char szTime[MAX_PATH] = {0};

	SYSTEMTIME time;
	::GetLocalTime(&time);

	/*sprintf_s(szTime, sizeof(szTime),
		"%04d-%02d-%02d %02d-%02d-%02d-%03d",
		time.wYear,time.wMonth,time.wDay,
		time.wHour,time.wMinute,time.wSecond, time.wMilliseconds
		);*/

	sprintf_s(szTime, sizeof(szTime),
		"%02d-%02d-%02d-%03d pos=%x",
		time.wHour,time.wMinute,time.wSecond, time.wMilliseconds, request_info_.startpos
		);

	strcat(szTime, ".mp4");

	return szTime;

#else
	char szTime[1024];
	time_t t;
	time(&t);
	tm* p = localtime(&t);

	timeval tv;
	gettimeofday(&tv, 0); //gettimeofday does not support TZ adjust on Linux.

	/*sprintf(szTime, "%4d-%02d-%02d %02d-%02d-%02d-%03ld",
		p->tm_year + 1900,
		p->tm_mon + 1,
		p->tm_mday,
		p->tm_hour,
		p->tm_min,
		p->tm_sec,
		tv.tv_usec / 1000
		);*/

	sprintf(szTime, "%02d-%02d-%02d-%03ld pos=%x",
		p->tm_hour,
		p->tm_min,
		p->tm_sec,
		tv.tv_usec / 1000,
		request_info_.startpos
		);

	strcat(szTime, ".mp4");
	return szTime;
#endif
}

std::string LocalHttpConnection::get_log_path(void)
{
	std::string log_path;
	std::tstring str_path = LocalHttpServer::Inst()->get_log_path();

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

	str_path += b2w(get_log_filename());
	log_path = w2b(str_path);
	return log_path;
}
