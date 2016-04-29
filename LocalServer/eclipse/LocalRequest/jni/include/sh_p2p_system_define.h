#ifndef _SHP2PSYSTEMDEFINE_H_
#define _SHP2PSYSTEMDEFINE_H_

#include <memory.h>

#define ARRAY_LEN_MINI   32
#define ARRAY_LEN_SHORT 64
#define ARRAY_LEN_MID	128
#define ARRAY_LEN_LONG	256

#ifdef WIN32

#ifdef SHP2PSYSTEMLIB_EXPORTS
#define SHP2P_API __declspec(dllexport)
#else
#define SHP2P_API __declspec(dllimport)
#endif

#define SHP2P_CALL __stdcall
#define SHP2P_CALLBACK __stdcall

#else
#define SHP2P_API
#define SHP2P_CALL
#define SHP2P_CALLBACK

#endif

//////////////////////////////////////////////////////////////////////////
//C Standards
//////////////////////////////////////////////////////////////////////////
#if defined(__STDC__)
# define PREDEF_STANDARD_C_1989
# if defined(__STDC_VERSION__)
#  define PREDEF_STANDARD_C_1990
#  if (__STDC_VERSION__ >= 199409L)
#   define PREDEF_STANDARD_C_1994
#  endif
#  if (__STDC_VERSION__ >= 199901L)
#   define PREDEF_STANDARD_C_1999
#  endif
# endif
#endif

//////////////////////////////////////////////////////////////////////////
// Pre-C89 compilers do not recognize certain keywords. 
// Let the preprocessor remove those keywords for those compilers.
//////////////////////////////////////////////////////////////////////////
#if !defined(PREDEF_STANDARD_C_1989) && !defined(__cplusplus)
# define const
# define volatile
#endif

//////////////////////////////////////////////////////////////////////////
// Define 8-bits Integer, 16-bits Integer,32-bits Integer
// All compliant compilers that support Standard C/C++
// VC++，ê? Borland C++, Turb C++  those who support C89,but undefined __STDC__) 
//////////////////////////////////////////////////////////////////////////
#if defined(__STDC__) || defined(__cplusplus) || defined(_MSC_VER) || defined(__BORLANDC__) ||  defined(__TURBOC__)
#include <limits.h>
// Defined 8 - bit Integer
#if defined(UCHAR_MAX) && (UCHAR_MAX == 0xFF)
typedef  char  sh_int_8, *sh_int_8_p; 
typedef  unsigned char sh_uint_8, *sh_uint_8_p;
#ifndef DEFINED_INT8
#define DEFINED_INT8
#endif
#endif
// Defined 16-bits Integer
#if defined(USHRT_MAX) && (USHRT_MAX == 0xFFFF)
typedef  short int  sh_int_16, *sh_int_16_p;
typedef  unsigned short int sh_uint_16, *sh_uint_16_p;
#ifndef DEFINED_INT16
#define DEFINED_INT16
#endif
#elif defined(UINT_MAX) && (UINT_MAX == 0xFFFF)
typedef  int  sh_int_16, *sh_int_16_p;
typedef  unsigned int sh_uint_16, *sh_uint_16_p;
#ifndef DEFINED_INT16
#define DEFINED_INT16
#endif	
#endif
// Defined 32-bits Integer
#if defined(UINT_MAX) && (UINT_MAX == 0xFFFFFFFFUL)
typedef int  sh_int_32, *sh_int_32_p;
typedef unsigned int sh_uint_32, *sh_uint_32_p;
#ifndef DEFINED_INT32
#define DEFINED_INT32
#endif
#elif defined(ULONG_MAX) && (ULONG_MAX == 0xFFFFFFFFUL)
typedef long int  sh_int_32, *sh_int_32_p;
typedef unsigned long int sh_uint_32, *sh_uint_32_p;
#ifndef DEFINED_INT32
#define DEFINED_INT32
#endif
#endif
#endif

//////////////////////////////////////////////////////////////////////////
// Define 64-bits Integer
// Here Only support typical systems 
// such as GNU/Linux Windows UNIX Vxworks  BSD Solaris 
//////////////////////////////////////////////////////////////////////////

// GNU/Linux System 64-bits Integer
#if defined(__GNUC__) || defined(linux) ||defined(__linux)
#if defined (__GLIBC_HAVE_LONG_LONG) || (defined(ULLONG_MAX) && (ULLONG_MAX == 0xFFFFFFFFFFFFFFFFUL)) || defined (PREDEF_STANDARD_C_1999)
typedef  long long sh_int_64, *sh_int_64_p;
typedef  unsigned long long sh_uint_64, *sh_uint_64_p;
#ifndef DEFINE_INT64
#define DEFINE_INT64
#endif
#endif
#endif

// Windows System 64-bits Integer
#if defined (WIN32) || defined (_WIN32)
#if defined(_MSC_VER) || defined(__BORLANDC__)
typedef __int64 sh_int_64, *sh_int_64_p;
typedef unsigned __int64 sh_uint_64, *sh_uint_64_p;
#ifndef DEFINE_INT64
#define DEFINE_INT64
#endif  
#elif !(defined(unix) || defined(__unix__) || defined(__unix))
typedef unsigned long long sh_int_64, *sh_int_64_p;
typedef signed long long sh_uint_64, *sh_int_64_p;
#ifndef DEFINE_INT64
#define DEFINE_INT64
#endif
#endif
#endif

// UNIX 
#if defined(unix) || defined(__unix__) || defined(__unix)
# define PREDEF_PLATFORM_UNIX
#endif
#if defined(PREDEF_PLATFORM_UNIX)
#include <unistd.h>
#if defined(_XOPEN_VERSION)
#if (_XOPEN_VERSION >= 3)
#define PREDEF_STANDARD_XOPEN_1989
#endif
#if (_XOPEN_VERSION >= 4)
#define PREDEF_STANDARD_XOPEN_1992
#endif
#if (_XOPEN_VERSION >= 4) && defined(_XOPEN_UNIX)
#define PREDEF_STANDARD_XOPEN_1995
#endif
#if (_XOPEN_VERSION >= 500)
#define PREDEF_STANDARD_XOPEN_1998
#endif
#if (_XOPEN_VERSION >= 600)
#define PREDEF_STANDARD_XOPEN_2003
typedef unsigned long long sh_uint_64, *sh_uint_64_p;
typedef signed long long sh_int_64, *sh_int_64_p;
#ifndef DEFINE_INT64
#define DEFINE_INT64
#endif
#endif
# endif
#endif

#ifndef NULL
#ifdef __cplusplus
#define NULL    0
#else
#define NULL    ((void *)0)
#endif
#endif

#ifdef WIN32
typedef wchar_t     sh_char;		
typedef wchar_t*    sh_char_p;	
#else
typedef char     sh_char;		
typedef char*    sh_char_p;	
#endif

/*平台类型，跟操作系统不是一个概念，操作系统详细信息在其他参数传入*/
enum SHPlatformType
{
    PLATFORM_NONE = -1,
    PLATFORM_PC,
    PLATFORM_MAC,
    PLATFORM_IPHONE,
    PLATFORM_IPAD,
    PLATFORM_ANDROID_PHONE,
    PLATFORM_ANDROID_PAD,
    PLATFORM_ANDROID_TV,
	PLATFORM_ANDROID_ROUTER,
    PLATFORM_ANDROID_BOX
};

/*用户上网类型*/
enum SHNetType
{
	kSHNet_None = 0,     //离线状态（断网)
	kSHNet_WireLine,	 //有线
	kSHNet_Wifi,		 //无线
	kSHNet_3G,			 //3G
	kSHNet_2G			 //2G
};

/*请求类型*//*上层请分开管理播放请求和下载请求*/
enum SHRequestType
{
	kSHRequest_Play = 0, //播放请求
	kSHRequest_Download	 //下载请求
};
/*
为了使得“跨平台P2P服务系统”尽可能独立于平台，操作系统相关的代码尽可能都放在调用P2P服务之前完成，
如果是P2P服务系统需要的参数，播放器进程获取后作为启动API的参数传入P2P服务系统即可。
*/
/*
如果是首次启动没有register_id时传入回调函数/P2P服务获取到register_id以后通知上层，
在pc平台将其写入注册表，其他平台按照自己的使用方式记录，以便以后启动时传入P2P系统
*/
/*参数必须赋值，char赋值null,ios上会给赋有效的默认值，导致后面不能根据是否为空等判断*/
typedef struct tagSHP2PSystemParam
{
	enum SHPlatformType	platform_type;			//平台类型
	sh_char_p		app_version;				//播放器版本号（P2P系统也会有一个自己的版本号）
    sh_char_p		app_update_version;         //升级程序版本号
    sh_char_p		p2psys_version;             //p2p 系统版本号
	sh_char_p		system_info;				//具体操作系统版本号等信息
	char*			machine_code;				//通过cpu硬盘等计算的机器码
	sh_char_p		app_path;					//应用程序运行路径
	sh_char_p		log_path;					//日志、dump等系统运行产生的文件目录地址
	bool			allow_log;					//是否打开全部日志
	sh_char_p		cache_path;					//缓存目录地址
    sh_char_p		install_time;               //安装时间
	sh_int_32		channel_id;					//渠道号
	sh_uint_32		register_id;				//在服务端注册的唯一ID
    char*			sohu_key;                   //注册密钥
	bool			allow_cache;				//是否允许缓存文件
	sh_int_32		cache_limit;				//缓存空间大小限制MB
	enum SHNetType	net_type;					//网络类型
	bool			allow_connect;				//是否允许联网		
	char*			local_ip;
	sh_int_32		report;
    bool            allow_preload_next;         //是否开启预加载下一集
    sh_int_32       upload_limit;               //用户上传速度限制B/s,0为不限速
	sh_int_32		download_limit;				//用户下载速度限制,单位KB/s,0为不限速
}SHP2PSystemParam;
//
enum SHVideoClarity
{
    kSHVideoClarity_Supper = 21,
	kSHVideoClarity_High = 1,
	kSHVideoClarity_Normal = 2,
    kSHVideoClarity_Auto,
	kSHVideoClarity_Original = 31
};
//注册成功通知
typedef void(SHP2P_CALLBACK *register_succeed_notify)(sh_int_32 id,sh_char_p sohu_key);
//收到视频数据通知, bool isheader 表示是否是头部数据
typedef void(SHP2P_CALLBACK *recv_video_data_notify)(sh_int_64 unique_id,sh_uint_32 index,/*SHRequestType request_type,*/sh_int_8_p data,sh_uint_32 len, bool isheader, sh_int_32 real_mp4_size);
//应答视频时长, 需要在此回调函数内把duration, size这两个数组的内容保存下来， 在回调函数完成后此内存会被删掉
typedef void(SHP2P_CALLBACK *get_video_duration_notify)(sh_int_64 unique_id, double* duration,  sh_uint_32_p size,  unsigned char (*hash_array)[32], sh_int_32 array_len);
//视频完成
typedef void(SHP2P_CALLBACK *video_finish_notify)(sh_int_64 unique_id);

enum SHP2pErrorType
{
	kSHP2pError_No,
	kSHP2pError_HotVers,              //请求HotVS出错， 请求HotVS是流程中第一个网络请求
	kSHP2pError_Dispatch,             //请求调度服务器出错
	kSHP2pError_VideoRequest,     	  //请求CDN视频数据出错
    kSHP2pError_Tracker,              
	kSHP2pError_Navigator,
    kSHP2pError_STUN,
#ifdef ENABLE_GATEWAY
    kSHP2pError_Gateway,
#endif // #ifdef ENABLE_GATEWAY
	kSHP2pError_Unknown
};

//视频数据请求错误通知
typedef void(SHP2P_CALLBACK *video_error_notify)(sh_int_64 unique_id,sh_uint_32 index,/*SHRequestType request_type,*/
												enum SHP2pErrorType error_type,sh_uint_32 error_code,sh_uint_32 status_code);

typedef void(SHP2P_CALLBACK *recv_push_message_notify)(sh_int_8* msg,sh_uint_32 msg_len);

/*参数必须赋值，不需要实现的请赋值NULL*/
typedef struct tagSHP2pSystemNofity
{
	register_succeed_notify		register_succeed_notify_proc; 
	recv_video_data_notify		recv_video_data_notify_proc; 
	get_video_duration_notify   get_video_duration_notify_proc;
	video_error_notify			video_error_notify_proc;
    video_finish_notify        video_finish_notify_proc;
	recv_push_message_notify	recv_push_message_notify_proc;
}SHP2pSystemNofity;

enum SHPeerStatus
{
    SHPeerStatus_Uninit = -1,
    SHPeerStatus_Fetching_Address = 0,
    SHPeerStatus_Requsting,
    SHPeerStatus_Punching,
    SHPeerStatus_PunchSucceed,
    SHPeerStatus_PunchFailed,
    SHPeerStatus_Connecting,
    SHPeerStatus_ConnectSucceed,
    SHPeerStatus_ConnectFailed,
    SHPeerStatus_Closed,
    SHPeerStatus_Pause,
    SHPeerStatus_Unused
};

//peer信息显示
typedef struct tagNewSHPeerInfo
{
    sh_uint_32 		id;
    sh_uint_32 		peerId;							//peer的注册id  
    sh_uint_32	    speed;							//速度
    sh_uint_32	    average_speed;					//平均速度
    enum SHPeerStatus	status;					    //状态
    char			hash[20];
    bool			iscdn;
    bool            need_punch;
    sh_uint_32 		fd;			
    sh_uint_32	    nat;							//peer的nat类型
    sh_uint_32	    tracker_id;      
    sh_uint_32 		ip;								//打通的IP	
    sh_uint_32	    port;							//打通的IP
    sh_uint_32	    punch_status;					//-1没有使用 0-成功 1-失败

	sh_uint_32      win_size;                       //窗口大小
	sh_uint_32      avg_rtt;                        //平均RTTS时间
	sh_uint_32      index_key;						//key值
	sh_uint_32      avg_send_count;                 //平均发送个数
	sh_uint_32		avg_receive_count;				//平均接受个数
	sh_uint_32		avg_timeout_count;				//平均超时个数
	double          lost_rate;                      //丢包率

    sh_uint_32	    fetching_address_time;          //获取外网地址时间
    sh_uint_32      file_request_time;              //文件交换时间
    sh_uint_32      punch_time;                     //打洞时间

    sh_char			strCdnIp[ARRAY_LEN_LONG];
    sh_char			strCdnKey[ARRAY_LEN_LONG];
    sh_char			strCdnUrl[ARRAY_LEN_LONG];
#ifdef ENABLE_FLASH_P2P
    bool            is_flash;
    sh_char         flash_id[ARRAY_LEN_MID];
#endif // #ifdef ENABLE_FLASH_P2P
} NewSHPeerInfo;

typedef struct tagNewSHDispInfo
{
    sh_char		    file_name[ARRAY_LEN_LONG];	   //视频名称
    sh_uint_32		num;						   //段号
    sh_uint_32		size;						   //文件大小
    double			speed;						   //瞬时速度
    double			average_speed;				   //平均速度
    double			percent;					   //进度
    double			p2p_percent;				   //带宽节约比
    sh_uint_32		cdn_num;					   //cdn数量
    sh_uint_64		start_time;					   //下载开始时间
    sh_uint_64		end_time;					   //下载结束时间
    sh_uint_32		state;						   //状态 0-下载完成 1-结束会话 2-下载出错
    sh_uint_32		oper;						   //0-新的下载，1-更新信息，2-下载完毕或出错（看state）						
    sh_uint_32		timespace;					   //消耗时间
    sh_uint_32		report_len;					   //上报文件长度
    sh_int_32       ID;
	sh_uint_32		type;						   //0:普通 1：跳转
	sh_uint_32      play_start;                    //播放开始位置
	sh_uint_32      duration;					   //文件时长
	sh_uint_32      byterate;                      //比特率
	sh_uint_32      state_code;                    //1:P2P 2:HTTP 3:P2P|HTTP 4:START 5:START|P2P 6:START|HTTP
	sh_uint_32      max_conn;                      //最大连接数
    NewSHPeerInfo	peer_list[ARRAY_LEN_SHORT];	   //最多显示ARRAY_LEN_MID个peer信息
    sh_uint_32      peer_list_len;
	sh_uint_64		uid;	                       //请求视频uid  

} NewSHDispInfo;

#endif
