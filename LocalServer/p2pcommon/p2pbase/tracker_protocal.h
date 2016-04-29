#ifndef _TRACKER_PROTOCAL_H
#define _TRACKER_PROTOCAL_H

#include "../base/common.h"

#define JSON_PRO
#pragma pack(push,1)
//以下结构中所有的ip和port均为网络字节序
//以下结构为1字节对齐
//公共头部，每个包都包含此公共头部
const int CMD_SOHU_MARK = 0x5ef83c2a;
typedef struct tagSHHeader
{
	uint16_t  packageLen;		//包长
	uint32_t  mark;           //标识
	uint16_t  cmd;            //命令字
}SHHeader;

#ifdef JSON_PRO
    const int CMD_LOG_IN = 0x9001;
#else
    const int CMD_LOG_IN = 0x8001;
#endif
//登录
typedef struct tagSHLogin
{
	SHHeader	header;
	uint16_t		protocal_version;	//协议版本
	uint32_t		client_version;
	uint32_t		update_version;
	uint32_t		localIp;
	uint32_t		id;					//注册id
	uint16_t		sp;					//new  
	uint16_t		city;				//new 
	char		natType;
}SHLogin;

//登录返回
typedef struct tagSHLoginResponse
{
	SHHeader	header;
	uint32_t		trackerId;
	uint32_t		fd;
	//test
// 	uint32_t		mapIp;
// 	short		mapPort;
}SHLoginResponse;

#ifdef JSON_PRO
    const int CMD_LOG_OUT = 0x9003;
#else
    const int CMD_LOG_OUT = 0x8003;
#endif
//注销,注销后请SERVER保证取消该用户的所有文件共享
typedef SHHeader SHLogout;
//注销返回
typedef SHHeader SHLogoutResponse;

#ifdef JSON_PRO
const int CMD_LIVE   = 0x9002;
#else
const int CMD_LIVE   = 0x8002;
#endif

//心跳,无须返回
typedef SHHeader SHLive;

#ifdef  JSON_PRO
    const int CMD_SHARE_FILE = 0x9004;
#else
    const int CMD_SHARE_FILE = 0x8024;
#endif
//文件共享
typedef struct tagSHShareFile
{
	SHHeader	header;
	uint16_t		file_num;	//共享的文件个数	
    uint16_t      type;
	char		hash[1];	//file_num*20，每个文件hash为20位
}SHShareFile;
//文件共享返回
typedef SHHeader SHShareFileResponse;

#ifdef JSON_PRO
    const int CMD_UN_SHARE_FILE = 0x9005;
#else
    const int CMD_UN_SHARE_FILE = 0x8005;
#endif
//取消共享
typedef struct tagSHUnShareFile
{
	SHHeader	header;
	uint16_t		file_num;	//取消共享的文件个数			
	char		hash[1];	//file_num*20，每个文件hash为32位
}SHUnShareFile;
//取消共享返回
typedef SHHeader SHUnShareFileResponse;

#ifdef JSON_PRO
const int CMD_FILE_SEARCH = 0x9006;
#ifdef ENABLE_FLASH_P2P
const int CMD_FILE_SEARCH_MIX = 0x8027;
#endif // #ifdef ENABLE_FLASH_P2P
#else
const int CMD_FILE_SEARCH = 0x8006;
#endif
//文件查询
typedef struct tagSHFileSearch
{
	SHHeader	header;	
	uint32_t		callId;
	char		hash[20];	//查询的文件hash值
}SHFileSearch;

#ifdef ENABLE_FLASH_P2P
const int32_t FLASH_PEER_ID_LEN = 64;
#endif // #ifdef ENABLE_FLASH_P2P
typedef struct tagSHFileSearchResult
{
	uint32_t		trackerId;  //
	uint32_t		fd;			//
	char		nat;		//peer的nat类型
	uint32_t		peerId;		//peer的注册id
#ifdef ENABLE_FLASH_P2P
	bool			is_flash;
	char			flash_peer_id[FLASH_PEER_ID_LEN];
#endif // #ifdef ENABLE_FLASH_P2P
}SHFileSearchResult;

//文件查询返回
typedef struct tagSHFileSearchResponse
{
	SHHeader			header;
	uint32_t				callId;
	char				hash[20];
	uint16_t				num;		//种子个数
	SHFileSearchResult	info[1];	//种子信息
}SHFileSearchResponse;

#ifdef JSON_PRO
const int CMD_FILE_REQUEST_CLIENT = 0x9007;
#else
const int CMD_FILE_REQUEST_CLIENT = 0x8007;
#endif

typedef struct tagSHPeer
{
	uint32_t trackerId;
	uint32_t fd;
	//test
	//uint32_t peerId;
}SHPeer;
//文件请求(client)
typedef struct tagSHFileRequestClient
{
	SHHeader	header;
	uint32_t		callId;
	char		hash[20];	//查询的文件hash值
	uint32_t		publicIp;
	short		publicPort;
	uint32_t		localIp;
	short		localPort;
	uint16_t		peernum;	//请求的peer个数
	SHPeer		peerinfo[1];//peer id信息
}SHFileRequestClient;

//文件请求返回(client)，可能需要多次返回
typedef struct tagSHFileRequestClientResponse
{
	SHHeader	header;
	uint32_t		callId;
	char		hash[20];	//查询的文件hash值
	uint32_t		publicIp;	//外网Ip
	uint16_t		publicPort;	//外网端口
	uint32_t		localIp;	//内网ip
	uint16_t		localPort;	//内网端口
	uint32_t		peerid;		//peer的注册id
}SHFileRequestClientResponse;

#ifdef JSON_PRO
    const int CMD_FILE_REQUEST_SERVER = 0x9008;
#else
    const int CMD_FILE_REQUEST_SERVER = 0x8008;
#endif

//文件请求(server)
typedef struct tagSHFileRequestServer
{
	SHHeader	header;
	uint32_t	callId;
	char		hash[20];	//查询的文件hash值
	uint32_t		trackerId;
	uint32_t		fd;
	uint32_t		publicIp;	//外网Ip
	uint16_t		publicPort;	//外网端口
	uint32_t		localIp;	//内网ip
	uint16_t		localPort;	//内网端口
	uint32_t		peerid;		//peer的注册id
	char		natType;
}SHFileRequestServer;

//文件请求返回(server)
typedef struct tagSHFileRequestServerResponse
{
	SHHeader	header;
	uint32_t		callId;
	char		hash[20];	//查询的文件hash值
	uint32_t		publicIp;	//外网Ip
	uint16_t		publicPort;	//外网端口
	uint32_t		localIp;	//内网ip
	uint16_t		localPort;	//内网端口
	uint32_t		trackerId;
	uint32_t		fd;
	//test
	//uint32_t		peerid;
}SHFileRequestServerResponse;


const int CMD_TRANSIT_CLIENT = 9;
//中转(client)
typedef struct tagSHTransitClient
{
	SHHeader	header;
	uint32_t		fd;			//
	short		trackerId;  //
	char		data[1];	//用户数据
}SHTransitClient;
//中转(client)回复
typedef SHHeader	SHTransitClientResponse;

const int CMD_TRANSIT_SERVER = 10;
//中转(SERVER)
typedef struct tagSHTransitServer
{
	SHHeader	header;
	uint32_t		id;			//peer id;
	char		data[1];	//用户数据
}SHTransitServer;
//中转(SERVER)返回
typedef SHHeader	SHTransitServerResponse;

//test
typedef SHHeader SHRegister;
typedef struct tagSHRegisterResponse
{
	SHHeader	header;
	uint32_t		id;			//注册id
}SHRegisterResponse;
const int CMD_REGISTER = 11;

typedef SHHeader SHFileBrowser;

typedef struct tagSHFileBrowserResponse
{
	SHHeader	header;
	int			fileNum;		
	char		hash[1];
}SHFileBrowserResponse;

const int CMD_FILE_BROWSER = 12;

#ifdef JSON_PRO
const int CMD_PUSH_SERVER = 0x7001;
#endif

#ifdef ENABLE_FLASH_P2P
const int FLASH_PEER_FLAG = 0x3E;
const int FLASH_EX_VERSION = 0x04;
const int SEARCH_IFOX_PEER_FLAG = 0x00;
const int SEARCH_IFOX_FLASH_PEER_FLAG = 0x01;
#endif // #ifdef ENABLE_FLASH_P2P
#pragma pack(pop)

#endif
