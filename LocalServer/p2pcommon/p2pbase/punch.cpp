#include "punch.h"
#include "../log/log.h"
#include "../base/algorithm.h"
#include "../base/timer.h"
#include "../base/socket_api.h"
#include "../base/threadpool.hpp"
#include "../sh_kernel.h"

using namespace boost::threadpool;

class CPunchImpl1 : public ISHPunchInterface
{
public:
	CPunchImpl1(){};
	virtual ~CPunchImpl1(){};
public:
	//等待对方探测，自己是公开或或者clone
    bool punch(const SHPunchParam& param, porting::EventHandle hEvent)
	{
        return porting::WaitForSingleObject(hEvent, SH_UDP_PUNCH_TIMEOUT);
	}
};

class CPunchImpl2 : public ISHPunchInterface
{
public:
	CPunchImpl2(){};
	virtual ~CPunchImpl2(){};
public:
	//向一个地址发送打洞包(对方不是对称型的)
    bool punch(const SHPunchParam& param, porting::EventHandle hEvent)
	{
		if(param.sock == INVALID_SOCKET)
			return false;
		SOCKADDR_IN addr;
		int32_t			len = sizeof(addr);
		addr.sin_family = AF_INET;
		SHPenchPackage package;
		package.len = sizeof(package);
		package.cmd = SHUdpCmd_Punch;
		int32_t num = 0;
		while (num++ < SH_PUNCH_RETRY_NUM)
		{
			addr.sin_port			  = param.nPeerLocalPort;
#ifdef WIN32
			addr.sin_addr.S_un.S_addr = param.nPeerLocalIp;
#else
            addr.sin_addr.s_addr = param.nPeerLocalIp;
#endif
            SocketAPI::sendto_ex(param.sock, (char *)&package, sizeof(package), 0, (sockaddr*)&addr, len);
			if(param.nPeerLocalIp != param.nPeerMapIp || param.nPeerLocalPort != param.nPeerMapPort)
			{
				addr.sin_port			  = param.nPeerMapPort;
#ifdef WIN32
                addr.sin_addr.S_un.S_addr = param.nPeerMapIp;
#else
                addr.sin_addr.s_addr = param.nPeerMapIp;
#endif
				SocketAPI::sendto_ex(param.sock, (char *)&package, sizeof(package), 0, (sockaddr*)&addr, len);
			}
            if(porting::WaitForSingleObject(hEvent, SH_UDP_PUNCH_TIMEOUT/SH_PUNCH_RETRY_NUM))
			{
				return true;
			}
		}
		return false;
	}
};

class CPunchImpl3 : public ISHPunchInterface
{
public:
	CPunchImpl3(){};
	virtual ~CPunchImpl3(){};
public:
	//向多个地址发送打洞包(对方是对称型)
    bool punch(const SHPunchParam& param, porting::EventHandle hEvent)
	{
		if(param.sock == INVALID_SOCKET)
			return false;
		SHPenchPackage package;
		package.len = sizeof(package);
		package.cmd = SHUdpCmd_Punch;
		SOCKADDR_IN addr;
		int32_t			len = sizeof(addr);
		addr.sin_family = AF_INET;
		int32_t num = 0;
		while (num++ < SH_PUNCH_RETRY_NUM)
		{
#ifdef WIN32
            addr.sin_addr.S_un.S_addr = param.nPeerLocalIp;
#else
            addr.sin_addr.s_addr = param.nPeerLocalIp;
#endif
			addr.sin_port			  = param.nPeerLocalPort;
			SocketAPI::sendto_ex(param.sock,(char *)&package,sizeof(package),0,(sockaddr*)&addr,len);
			for (int32_t i = 0; i <= 5; ++i)
			{
#ifdef WIN32
                addr.sin_addr.S_un.S_addr = param.nPeerMapIp;
#else
                addr.sin_addr.s_addr = param.nPeerMapIp;
#endif
				addr.sin_port	= static_cast<uint16_t>(param.nPeerMapPort+i);
				SocketAPI::sendto_ex(param.sock,(char *)&package, sizeof(package), 0, (sockaddr*)&addr, len);
				addr.sin_port	= static_cast<uint16_t>(param.nPeerMapPort-i);
				SocketAPI::sendto_ex(param.sock,(char *)&package, sizeof(package), 0, (sockaddr*)&addr, len);
			}
            if(porting::WaitForSingleObject(hEvent, SH_UDP_PUNCH_TIMEOUT/SH_PUNCH_RETRY_NUM))
			{
				return true;
			}
		}
		return false;
	}
};

CSHPunch::CSHPunch(void)
{
	punch_succeed_event_.reset();
	is_running_	= true;
}

CSHPunch::~CSHPunch(void)
{
}

ISHPunchInterface *CSHPunch::get_punch_obj(bool sponsor,SHNatType selfNatType,SHNatType peerNatType)
{
	DEBUG_LOG("udptrace", _T("sponsor = %d,selfNatType = %d,peerNatType = %d\n"), sponsor,selfNatType,peerNatType);
	if(selfNatType == NatType_ERROR || peerNatType == NatType_ERROR)
	{
		DEBUG_LOG("udptrace", _T("One natType error,can't punch,GetPunchObj failed\n"));
		return NULL;
	}
	if(selfNatType == NatType_BLOCKED || peerNatType == NatType_BLOCKED)
	{
		DEBUG_LOG("udptrace", _T("One natType is blocked,can't punch,GetPunchObj failed\n"));
		return NULL;
	}
	if((selfNatType == NatType_FULL_CONE || selfNatType == NatType_OPEN_INTERNET) && 
		(peerNatType == NatType_FULL_CONE || peerNatType == NatType_OPEN_INTERNET))
	{
		if(sponsor)
		{
			DEBUG_LOG("udptrace", _T("self/peer natType is open or fullcone and self is sponsor,select punch obj is CPunchImpl2\n"));
			return new CPunchImpl2();
		}
		else
		{
			DEBUG_LOG("udptrace", _T("self/peer natType is open or fullcone and self is not sponsor,select punch obj is CPunchImpl\n"));
			return new CPunchImpl1();
		}
	}
	if(selfNatType == NatType_FULL_CONE || selfNatType == NatType_OPEN_INTERNET)
	{
		DEBUG_LOG("udptrace", _T("self natType is open or fullcone ,select punch obj is CPunchImpl1\n"));
		return new CPunchImpl1();
	}
	if(peerNatType == NatType_FULL_CONE || peerNatType == NatType_OPEN_INTERNET)
	{
		DEBUG_LOG("udptrace",  _T("peer natType is open or fullcone ,select punch obj is CPunchImpl2\n"));
		return new CPunchImpl2();
	}
	if(peerNatType == NatType_SYMMETRIC_NAT)
	{
		DEBUG_LOG("udptrace", _T("peer natType is SYMMETRIC ,select punch obj is CPunchImpl3\n"));
		return new CPunchImpl3();
	}
	else
	{
		DEBUG_LOG("udptrace", _T("select punch obj is CPunchImpl2\n"));
		return new CPunchImpl2();
	}
}

bool  CSHPunch::punch(const SHPunchParam& param, int32_t& punchSucceedIp, int16_t& punchSucceedPort,int32_t& punch_use_time)
{
	if(punch_succeed_event_.get() != NULL)
		return false;
	int64_t now = get_tick_count();
	//获取原超时时间
#ifdef WIN32
    uint32_t old_timeout = 0;
#else
    struct timeval old_timeout;
    old_timeout.tv_sec = 0;
    old_timeout.tv_usec = 0;
#endif
	unsigned int  optlen		= 0;
    SocketAPI::getsockopt_ex(param.sock, SOL_SOCKET, SO_RCVTIMEO, (void *)&old_timeout, &optlen);
	bool result = false;
	punch_socket_		= param.sock;
    punch_succeed_event_= porting::CreateEvent();
	SHKernel::instance()->shedule_recv_proc(boost::bind(&CSHPunch::recv_proc, shared_from_this()));

    boost::shared_ptr<ISHPunchInterface> punchPtr_(get_punch_obj(param.sponsor, param.selfNatType, param.peerNatType));
	if(punchPtr_.get() == NULL)
	    return false;
	result = punchPtr_->punch(param, punch_succeed_event_);
	is_running_ = false;
	//sh_thread_pool().wait(2);
    porting::CloseHandle(punch_succeed_event_);

	//打洞成功后可能对方的外网端口已经变了
	if(result)
	{
		punchSucceedIp	=	peer_ip_;
		punchSucceedPort=	peer_port_;
	}
    SocketAPI::setsockopt_ex(param.sock,SOL_SOCKET, SO_RCVTIMEO, (void *)&old_timeout, sizeof(old_timeout));
	punch_use_time= static_cast<int32_t>(get_tick_count()-now);
	DEBUG_LOG("udptrace", _T("punch %s,used time = %d ms\n"), (result ? _T("succeed") : _T("failed")) ,punch_use_time);
	return result;
}

void  CSHPunch::recv_proc()
{
#ifdef WIN32
    uint32_t timeout = SH_UDP_PUNCH_TIMEOUT;
#else
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = SH_UDP_PUNCH_TIMEOUT * 100;
#endif

	int32_t result = SocketAPI::setsockopt_ex(punch_socket_, SOL_SOCKET, SO_RCVTIMEO, (void *)&timeout, sizeof(timeout));
	if(result == false)
		return;
	char szData[1024]= "";
	int32_t  dataLen         = sizeof(szData);
    std::string recvData;
	while(is_running_)
	{
		SOCKADDR_IN addr	= {0};
		uint32_t			len		= sizeof(SOCKADDR_IN);
		int32_t			recvLen = 0;
        recvLen = SocketAPI::recvfrom_ex(punch_socket_, szData, dataLen, 0, (sockaddr *)&addr, &len);

#ifdef WIN32
        if (recvLen <= 0 && WSAGetLastError() != 10054 && WSAGetLastError() != 10035)
#else 
        if (recvLen <= 0 && errno != ECONNRESET && errno != EWOULDBLOCK)
#endif
        {
            return;
        }

		if(recvLen <= 0)
		{
            continue;
		}

		DEBUG_LOG("udptrace",  _T("recv Punch DataLen = %d\n"), recvLen);
        if (recvLen > 1024)
            return;

		recvData = string(szData, recvLen);
		if(recvData.size() < sizeof(SHUdpHeader))
		{
			DEBUG_LOG("udptrace", _T("recv dataLen < %d(HeaderLen),continue\n"), sizeof(SHUdpHeader));
			continue;
		}
		SHUdpHeader* udpPtr = (SHUdpHeader*)recvData.data();
		if(recvData.size() < (size_t)udpPtr->len)
		{
			DEBUG_LOG("udptrace", _T("recv dataLen < %d(PackageLen),continue\n"),udpPtr->len);
			continue;
		}
		switch (udpPtr->cmd)
		{
		case SHUdpCmd_Punch:
			{
				//收到打洞包，发送打洞回复包
				//设置接收超时时间
#ifdef WIN32
                uint32_t second_timeout =  SH_UDP_PUNCH_TIMEOUT / 2;
#else
                struct timeval second_timeout;
                second_timeout.tv_sec = 0;
                second_timeout.tv_usec = SH_UDP_PUNCH_TIMEOUT / 2;
#endif
                SocketAPI::setsockopt_ex(punch_socket_, SOL_SOCKET,SO_RCVTIMEO,(void *)&second_timeout, sizeof(second_timeout));
				SHPenchPackageRsp package;
				package.cmd = SHUdpCmd_Punch_Rsp;
				int32_t sendLen = SocketAPI::sendto_ex(punch_socket_, (char *)&package, sizeof(package), 0, (sockaddr *)&addr, len);
				sendLen		= SocketAPI::sendto_ex(punch_socket_, (char *)&package,sizeof(package),0,(sockaddr *)&addr, len);
				DEBUG_LOG("udptrace", _T("recv SHUdpCmd_Punch,send SHUdpCmd_Punch_Rsp,ip: %s,port: %d\n"),
					b2w(inet_ntoa(addr.sin_addr)).c_str(),ntohs(addr.sin_port));
				break;
			}
		case SHUdpCmd_Punch_Rsp:
			{
				//收到打洞回复包，认为打洞成功了，发送打洞回复回复包
				SHPenchPackageRspRsp package;
				package.cmd = SHUdpCmd_Punch_Rsp_Rsp;
				SocketAPI::sendto_ex(punch_socket_, (char *)&package, sizeof(package),0,(sockaddr *)&addr,len);
				SocketAPI::sendto_ex(punch_socket_, (char *)&package,sizeof(package),0,(sockaddr *)&addr,len);
				peer_port_ = addr.sin_port;
#ifdef WIN32
				peer_ip_   = addr.sin_addr.S_un.S_addr;
#else
                peer_ip_   = addr.sin_addr.s_addr;
#endif
                porting::SetEvent(punch_succeed_event_);
				DEBUG_LOG("udptrace", _T("recv SHUdpCmd_Punch_Rsp,punch succeed,send SHUdpCmd_Punch_Rsp_Rsp,ip: %s,port: %d\n"),
					b2w(inet_ntoa(addr.sin_addr)).c_str(), ntohs(addr.sin_port));
				return;
				//break;
			}
		case SHUdpCmd_Punch_Rsp_Rsp:
			{
				//收到打洞回复回复包，恭喜打洞成功了
				peer_port_ = addr.sin_port;
#ifdef WIN32
                peer_ip_   = addr.sin_addr.S_un.S_addr;
#else
                peer_ip_   = addr.sin_addr.s_addr;
#endif
                porting::SetEvent(punch_succeed_event_);
				DEBUG_LOG("udptrace", _T("recv SHUdpCmd_Punch_Rsp_Rsp,punch succeed,ip: %s,port: %d\n"),
					b2w(inet_ntoa(addr.sin_addr)).c_str(), ntohs(addr.sin_port));
				return;
			}
		default:
			DEBUG_LOG("udptrace",  _T("recv UnKnown type package\n"));
			recvData.clear();
			break;
		}
		recvData.erase(0, udpPtr->len);
	}
}

