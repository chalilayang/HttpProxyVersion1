#include "local_http_connection.h"
#include "local_http_server.h"
#include "../p2pcommon/log/log.h"
#include "../p2pcommon/base/algorithm.h"
/*#include "p2p_tool.h"*/

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

void LocalHttpConnection::Init()
{
	userid_uid_map_.clear();
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

void LocalHttpConnection::Close()
{
	if (false == is_open_) return;
	
	userid_uid_map_.clear();
	is_open_ = false;

	if (http_service_)
	{
		http_service_->stop();
		//http_service_.reset();
	}
}


LocalHttpConnection::~LocalHttpConnection()
{
	userid_uid_map_.clear();
}

void LocalHttpConnection::on_service_close()
{
    //EVENT_LOG("kernel", _T("http_service had been completely closed.\n"));
	if (!local_http_server_.expired())
	{
		local_http_server_.lock()->on_connection_close(shared_from_this());
	}

    //LocalHttpServer::Inst()->OnConnectionClose(shared_from_this());
}

void LocalHttpConnection::on_service_read(const boost::system::error_code& ec, HttpRequest::HttpRequestPtr p_request, IOBuffer content_buf)
{
	if (!is_open_ || local_http_server_.expired())
	{
		return;
	}
	
    if (!ec && p_request)
	{
		string command = p_request->get_path();
		std::tstring strLog = b2w(command);
		INFO_LOG(LOCAL_SERVER_LOG, _T("解析 http 请求: %s\n"), strLog.c_str());

		if (command.find("/crossdomain.xml")!= string::npos)
		{
			std::string str_resp(FLASH_CROSS_DOMAIN_HEADER);
			response200_xml(str_resp);
		}
		else if (command.find("/shakehand") != string::npos)
		{
			std::string str_resp("sharkhand ok\0");
			response200_plain(str_resp);
			//Close();			
		}
		//else if (command.find("/close") != string::npos)
		//{
		//	local_http_server_.lock()->on_connection_close(shared_from_this());
		//	Close();
		//}
		//else if (command.find("/notify_buffer") != string::npos)
		//{
		//	string uid("");
		//	int canplaytime(0);
		//	if(!parse_notify_buffer(command,uid,canplaytime))
		//	{
		//		return;
		//	}

		//	response200_plain(std::string(""));
		//	map<string,int32_t>::iterator it = userid_uid_map_.find(uid);
		//	if(it != userid_uid_map_.end())
		//	{
		//		notify_buffer(it->second,canplaytime);
		//	}
		//}
		//else if (command.find("/flashp2p") != string::npos)
		//{
		//	//TODO
		//}
		else if (parse_flash_request(command))
		{
		  	start_download();
		}

		if (http_service_)
		{
			http_service_->read_header();
		}
	}
	else
	{
		INFO_LOG(LOCAL_SERVER_LOG, _T("读取HTTP请求数据发生错误! error: %d, %s, p_request = %d\n"), ec.value(), b2w(ec.message()).c_str(), p_request.get());
		Close();
	}
}

void LocalHttpConnection::on_service_read_timeout()
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
		INFO_LOG(LOCAL_SERVER_LOG, _T("sevice write error: %d, %s\n"), ec.value(), b2w(ec.message()).c_str());
		if (erro_count_ > 3 || ec.value() == 10053 || ec == boost::asio::error::operation_aborted || ec == boost::asio::error::eof||ec == boost::asio::error::connection_reset)
		{
			Close();
			erro_count_ = 0;
		}
	}
}

void LocalHttpConnection::response200_plain(std::string &str_msg)
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

bool LocalHttpConnection::parse_flash_request(string data)
{
	download_info_.init();
	size_t pos = 0;
	if(data.size() > 0 && data[0] == '/')
		data.erase(0,1);

	//type
	if((pos = data.find("?",0)) == string::npos)
		return false;
	download_info_.type = data.substr(0,pos);
	if (download_info_.type.empty())
	{
		return false;
	}

	data.erase(0,pos+1);
	std::vector<string> strs = SpliterString(data,"&");
	string userid("");
	for (size_t i = 0; i < strs.size(); ++i)
	{
		vector<string> attri = SpliterString(strs[i],"=");
		if(attri.size() != 2)
			continue;
		trim_str(attri[1]);
		if(attri[0] == "dnum")
		{
			download_info_.dnum = atoi(attri[1].c_str());
		}
		else if(attri[0] == "pnum")
		{
			download_info_.pnum = atoi(attri[1].c_str());
		}
		else if(attri[0] == "vid")
		{
			download_info_.vid= atoi(attri[1].c_str());
		}
		else if(attri[0] == "start")
		{
			download_info_.start= atoi(attri[1].c_str());
		}
		else if(attri[0] == "startpos")
		{
			download_info_.startpos= atoi(attri[1].c_str());
		}
		else if (attri[0].compare("definition") == 0)
		{
			download_info_.definition = atoi(attri[1].c_str());
		}
		else if (attri[0].compare("ismytv") == 0)
		{
			download_info_.isMy = bool(atoi(attri[1].c_str()));
		}
	}

	if(!userid.empty() && download_info_.vid != 0)
	{
		int32_t uid=((sh_int_64)download_info_.isMy) << 32 | download_info_.vid;
		userid_uid_map_.insert(make_pair(userid,uid));
	}

	if(download_info_.vid == 0)
		return false;

	return true;
}

bool LocalHttpConnection::parse_notify_buffer(string data,string& userid,int& canPlayTime)
{
	size_t pos  = 0;
	canPlayTime = 0;
	if(data.size() > 0 && data[0] == '/')
		data.erase(0,1);
	if((pos = data.find("?",0)) == string::npos)
		return false;
	data.erase(0,pos+1);

	vector<string> strs = SpliterString(data,"&");
	for (size_t i = 0; i < strs.size(); ++i)
	{
		vector<string> attri = SpliterString(strs[i],"=");
		if(attri.size() != 2)
			continue;
		trim_str(attri[1]);
		if(attri[0] == "uuid")
		{
			userid	= attri[1];
		}
		else if(attri[0] == "cptime")
		{
			canPlayTime = atoi(attri[i].c_str());
		}
	}

	string strError;
	if(userid.empty())
		strError = "uuid为空";
	return strError.empty();

}

void LocalHttpConnection::send_data(std::string &str_data, int len, bool isheader /*= false*/, sh_int_32 real_mp4_size /*= 0*/)
{
	if (!is_open_ || local_http_server_.expired() || http_service_.get() == NULL)
	{
		return;
	}

	if (isheader)
	{
		ostringstream stream;
		stream<<"HTTP/1.1 200 OK\r\nServer: p2plocalserver\r\nContent-Type: video/mp4\r\nContent-Length: ";
		stream<<real_mp4_size<<"\r\nConnection: close\r\n\r\n";
		IOBuffer str_response(stream.str());
		http_service_->write(str_response);
	}

	IOBuffer str_response(str_data);
	http_service_->write(str_response);
}


void LocalHttpConnection::start_download()
{
	if (local_http_server_.expired())
	{
		return;
	}

	if (download_info_.type.compare("play") == 0)
	{
		//播放
		sh_int_64 uid = local_http_server_.lock()->start_request_video_data(download_info_.vid,/*kSHVideoClarity_High*/get_video_definiton((VideoVersion)download_info_.definition),download_info_.dnum,download_info_.isMy, download_info_.pnum);
		//sh_int_64 uid = start_request_video_data_time_ex(download_info_.vid, kSHVideoClarity_High, download_info_.isMy, download_info_.start, 0);
		INFO_LOG(LOCAL_SERVER_LOG, _T("开始下载第 %d 段数据, vid = %d\r\n"), download_info_.dnum, download_info_.vid);
		local_http_server_.lock()->on_connection_start(uid, shared_from_this());
	}
	else if (download_info_.type.compare("seek") == 0)
	{
		//拖拽
	}
	

	//if(download_info_.start == 0 && download_info_.startpos == 0)
	//{
	//	//get_video_duration(download_info_.vid,kSHVideoClarity_High,download_info_.isMy);
	//	int uid = start_request_video_data(download_info_.vid,kSHVideoClarity_High,download_info_.dnum,download_info_.isMy, download_info_.pnum);
	//}
	//else if(download_info_.start != 0)
	//{
	//	get_video_duration(download_info_.vid,kSHVideoClarity_High,download_info_.isMy);
	//	start_request_video_data_time_ex(download_info_.vid,kSHVideoClarity_High,download_info_.isMy,download_info_.start, download_info_.pnum);
	//}
	//else if(download_info_.startpos != 0)
	//{
	//	get_video_duration(download_info_.vid,kSHVideoClarity_High,download_info_.isMy);
	//	notify_play_num(download_info_.vid,download_info_.pnum);
	//	start_request_video_data_range(download_info_.vid,kSHVideoClarity_High,download_info_.dnum,download_info_.isMy,download_info_.startpos,download_info_.size);
	//	
	//}
}

SHVideoClarity LocalHttpConnection::get_video_definiton(VideoVersion definiton)
{
	SHVideoClarity defi = kSHVideoClarity_High;
	switch (definiton)
	{
	case VER_HIGH:
		defi = kSHVideoClarity_High;
		break;
	case VER_NORMAL:
		defi = kSHVideoClarity_Normal;
		break;
	case VER_SUPER:
		defi = kSHVideoClarity_Supper;
		break;
	case VER_ORIGINAL:
		defi = kSHVideoClarity_Original;
		break;
	default:
		defi = kSHVideoClarity_Auto;
	}

	return defi;
}
