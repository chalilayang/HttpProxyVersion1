#ifndef _UDP_DEF_H_
#define _UDP_DEF_H_

#include <string>
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>
#include "../base/common.h"
#include "../p2pbase/player_def.h"
#include "../base/socket_api.h"

using namespace std;
#pragma pack(push,1)

#define  IS_SET_BIT(c, n)  ((c) & (1 << (n-1))) > 0 
#define  SET_BIT(c, n)  (c) = (c) | (1 << (n-1))

#define  REQ_SUBPIECE_CHECK_FLAG  1
#define  RES_SUBPIECE_CHECK_FLAG   1


#define  SH_UDP_PUNCH_TIMEOUT	(5000)	//udp打洞超时时间5S
#define  SH_PUNCH_RETRY_NUM		10

//
enum SHNatType
{
    NatType_ERROR=0,			
    NatType_BLOCKED,			
    NatType_OPEN_INTERNET,		//Open型，一般是公网IP
    NatType_FULL_CONE,			//CONE 型
    NatType_PORT_RESTRICTED,	//端口限制型
    NatType_RESTRICTED,			//IP限制型
    NatType_FIREWALL,			//防火墙，一般是公网IP，但是有本地防火墙
    NatType_SYMMETRIC_NAT		//对称型
};

enum SHUdpCmdType
{
	SHUdpCmd_Unknown = 0,
	SHUdpCmd_Punch = 9999,
	SHUdpCmd_Punch_Rsp,
	SHUdpCmd_Punch_Rsp_Rsp,
	SHUdpCmd_Punch_Msg,
	SHUdpCmd_File_Info,
	SHUdpCmd_Request,
	SHUdpCmd_Response,
	SHUdpCmd_Close,
	SHUdpCmd_Overload,
	SHUdpCmd_UNLIMIT
};

//
typedef struct tagSHUdpHeader
{
	uint32_t		len;
	int		        cmd;
	char	encrypt;	//0 不加密
	tagSHUdpHeader()
	{
		memset(this,0,sizeof(tagSHUdpHeader));
	}
}SHUdpHeader;

typedef struct tagSHUdpMsg
{
	SHUdpHeader header;
	char		msg[1];
	tagSHUdpMsg()
	{
		memset(this,0,sizeof(tagSHUdpMsg));
	}
}SHUdpMsg;


typedef struct tagSHUdpFileInfo
{
	SHUdpHeader header;
	int			size;
	char		name[1];
	tagSHUdpFileInfo()
	{
		memset(this,0,sizeof(tagSHUdpFileInfo));
	}
}SHUdpFileInfo;

typedef struct tagSHUdpRequest
{
	SHUdpHeader header;
	int			start;
	int 		len;
	tagSHUdpRequest()
	{
		memset(this,0,sizeof(tagSHUdpRequest));
	}
}SHUdpRequest;

typedef struct tagSHUdpResponse
{
	SHUdpHeader header;
	int			start;
	int 		len;
	char		data[1];
	tagSHUdpResponse()
	{
		memset(this,0,sizeof(tagSHUdpResponse));
	}
}SHUdpResponse;

typedef struct tagSHSubpieceRequest
{
	SHUdpHeader header;
	int 		piece_index;
	int  		subpiece_index;
	int 		len;
	tagSHSubpieceRequest()
	{
		memset(this,0,sizeof(tagSHSubpieceRequest));
	}
}SHSubpieceRequest;

typedef struct tagSHSubpieceResponse
{
	SHUdpHeader header;
	int 		piece_index;
	int  		subpiece_index;
	int			len;
	char		data[1];
	tagSHSubpieceResponse()
	{
		memset(this,0,sizeof(tagSHSubpieceResponse));
	}
}SHSubpieceResponse;

//
typedef SHUdpHeader	SHPenchPackage;
typedef SHUdpHeader	SHPenchPackageRsp;
typedef SHUdpHeader	SHPenchPackageRspRsp;
typedef SHUdpHeader	SHUdpClose;
typedef SHUdpHeader SHUdpOverload;
typedef SHUdpHeader SHUdpUnlimit;

typedef struct tagSHPunchParam
{
	SOCKET sock;
	int    nPeerMapIp;
	short  nPeerMapPort;
	int	   nPeerLocalIp;
	short  nPeerLocalPort;
	SHNatType selfNatType;
	SHNatType peerNatType;
	bool   sponsor;
	tagSHPunchParam()
	{
		memset(this,0,sizeof(tagSHPunchParam));
	}
}SHPunchParam;

enum SHPunchConnectResult
{
	SHPunchConnect_PunchSucceed = 0,
	SHPunchConnect_PunchFailed,
	SHPunchConnect_ConnectSucceed,
	SHPunchConnect_ConnectFailed
};

typedef boost::function<bool(SOCKET, uint32_t, uint16_t)> SH_MAPIP_CALLBACK;

typedef boost::function<void(SHPunchConnectResult, int32_t, int16_t,int32_t)>   SH_PUNCH_CALLBACK;

typedef boost::function<bool(SHNatType)> SH_NAT_DETECT_CALLBACK;

#pragma pack(pop)
#endif
