#ifndef PIAYER_DEFINE_H_
#define PIAYER_DEFINE_H_

#include "../base/common.h"
#include "sh_p2p_system_define.h"

using std::tstring;
using  std::string;

//视频版本
enum VideoVersion
{
	VER_ERROR	 = -1,
	VER_HIGH	 = 1,     //高清版
	VER_NORMAL   = 2,     //普清版
	VER_SUPER	 = 21 ,   //超清版
	VER_ORIGINAL = 31     //原画版
};


enum SHDType
{
  SHDTYPE_NORMAL = 0,
  SHDTYPE_PRELOAD,
  SHDTYPE_PRELOAD_NEXT,
  SHDTYPE_DOWNLOAD,
  SHDTYPE_CLIENT_PLAY,
  SHDTYPE_LIVE_PLAY,
  SHDTYPE_TEST_SPEED,
  SHDTYPE_USER_TEST_SPEED,
  SHDTYPE_SERVER_TEST_SPEED,
  SHDTYPE_MERGED_MP4_PLAY,
  SHDTYPE_MERGED_MP4_DOWNLOAD,
  SHDTYPE_PUSH_DOWNLOAD
};

//看点信息
struct SHVideoAspect
{
  int32_t				time;	//时间
  tstring	        desc;	//描述
};

struct SHVideoSection
{
  double			duration;//时常
  int32_t			size;	 //大小
  tstring	      url;	 //视频各片段文件地址
  tstring	      hashId;		 //hashId
  tstring	      key;		 //视频加密串, 防盗链
  tstring	      newAddress;	 //新架构视频片段地址
};

struct SHNetInfo
{
public:
	int32_t				p2pflag;	//传给加速器的参数
	int32_t				vid;		//视频id
	int32_t				norVid;		//对应的普清VID
	int32_t				highVid;	//对应的高清VID
	int32_t				superVid;	//对应的超清VID
	int32_t              oriVid;     //原画vid    
	bool			        longVideo;	//是否是长视频
	int32_t				tn;			//请求调度时转发给调度服务器(参数名:cdn)
	int32_t				status;		//视频信息状态, 1为正常
	bool			play;		//1为正常播放 0为禁播
	bool			fms;		//是否为FMS视频源
	bool			fee;		//是否为付费视频
	int32_t				pid;
	int32_t				mypid;		//播客专辑视频的pid
	int32_t				cid;
	int32_t				playerListId;
	int32_t				fps;		//视频桢率
	int32_t				version;	//视频版本, 1为高清 2为流畅
	int32_t				num;		//该视频在专辑中的位置
	int32_t				st;			//片头时长, 跳片头使用
	int32_t				et;			//片尾时长, 跳片尾使用
	int32_t              systype;    // 0:vrs类型 1：bms类型
	int32_t				width;		//视频宽
	int32_t				height;		//视频高
	tstring			    v_name;		//视频名称
	tstring			    ch;			//视频所属频道
	tstring			    allot;		//调度服务器地址
	tstring			    reserveIp;	//备用调度地址，以;分开，如:220.181.61.229;115.25.217.132
	tstring			    url;		//视频最终播放页
	tstring              coverImg;   //视频封面图
    std::vector<SHVideoAspect>	    aspects; //看点
    std::vector<SHVideoSection>	sections;//视频段信息
	std::tstring        catcode; //vrs中的新分类代码
    int32_t              vidFromWeb;	 //从页面传过来的vid
    int32_t		        downloadFlag;  //2-不允许下载,允许预加载, 1-允许下载, 0-不允许下载和预加载
    bool				    bExpired;	// 即将到期

    //mytv 特有
    tstring           cmscat;
    double          totalDuration;
    int32_t          ktvId;
    int32_t          tvid;
	//above from server
    bool isMy;

	SHNetInfo()
	{
		p2pflag		= 0;
		vid			    = 0;
		norVid		= 0;
		highVid		= 0;
		superVid	= 0;
		oriVid		  = 0;
		longVideo = false;
		tn			    = 0;
		status	  = 0;
		play			= false;
		fms			  = false;
		fee			  = false;
		pid			  = 0;
		mypid		= 0;
		fps			  = 0;
		version	= 0;
		num			= 0;
		st				= 0;
		et				= 0;
		cid			= 0;
		playerListId		= 0;
		width		= 0;
		height		= 0;
        vidFromWeb   = -1;

        isMy = false;
	}
};

typedef boost::shared_ptr<SHNetInfo> SHNetInfoPtr;

inline bool is_piracy(const std::string& source)
{
  return  !source.empty() && (source.at(0) =='x' || source.at(0) == 'X');
}


typedef boost::shared_ptr<NewSHPeerInfo>  SHPeerInfoPtr;

struct SHCDNInfo
{
  tstring	  ip;       
  tstring 	key;
  tstring	  url;
  int32_t		idc;
  bool       isp2p;       //免费CDN标记, false 为 free cdn

  SHCDNInfo() {};
  SHCDNInfo(const tstring& strIp, const tstring& strKey,const tstring& strUrl, int32_t i = 0, bool p2p = true)
  {
    ip	= strIp;
    key	= strKey;
    url = strUrl;
    idc = i;
    isp2p = p2p;
  }

  bool operator==(const SHCDNInfo &sp) const
  {
    return ip == sp.ip && key == sp.key && url == sp.url && idc == sp.idc && isp2p == sp.isp2p;
  }
};

typedef boost::shared_ptr<SHCDNInfo> SHCDNInfoPtr;

struct SHVodInfo
{
  tstring   ip;
  tstring		uri;
  tstring		key;
  tstring		hashId;
  tstring		filename;
  int64_t		uniqID;	
  tstring		newcdn;
  int32_t	plat;
  tstring		ch;
  tstring		catcode;
  std::vector<string>	rips;		//备用调度IP
  int32_t			liveid;			//直播视频ID
  int32_t			videoid;		//视频ID编号
  int32_t			nextid;			//下一集视频ID
  int32_t			dnum;			//文件片段编号
  int32_t			pnum;			//当前播放段
  double			dtime;
  double			ptime;
  int32_t			filesize;		//文件大小
  double			play_start;			//文件开始请求播放
  int32_t            cdnnum;
  int32_t			idc;
  int32_t			p2pflag;
  int32_t			dl_startPos;
  int32_t            dl_endPos;
  int32_t			duration;       //s
  int32_t           pos_in_file;    //只使用于大mp4,其它情况无效
  int64_t           call_id;        //回调到localserver id,用于找出对应http connection 
  SHDType		    shdtype;			//下载类型 0默认,1预加载,2下载,3预加载下一集
  bool		downloadFromP2p;
  bool		reportLocalData;

  int32_t            ad_duration_;

  int32_t            nettype_;
  int32_t            area_;
  void reset()
  {
    p2pflag = 0;
    pnum	= 0;
    dnum	= 0;
    dtime	= 0;
    ptime	= 0;
    filesize= 0;
    play_start	= 0;
    liveid = 0;
    videoid = 0;
    nextid =  0;
    cdnnum  = 0;
    idc		= 0;
    dl_startPos=0;
    dl_endPos = 0;
    duration=0;
    shdtype	= SHDTYPE_NORMAL;
    downloadFromP2p = false;
    reportLocalData = true;

    ad_duration_ = 0;
    nettype_ = 0;
    area_ = 0;
  }
  SHVodInfo()
  {
    reset();
  }
};

typedef boost::shared_ptr<SHVodInfo>  SHVodInfoPtr;

//下载的统计信息
struct SHDownloadStat
{
  int32_t		startTime;
  int32_t     endTime;
  int32_t		realFileSize;
  double	  speed;			//瞬时速度	KB/S
  double	  averageSpeed;	//平均速度  KB/S
  double	  percentage;		//当前下载进度
  double   cdnSpeed;
  double	  peerSpeed;

  double    cdnAvgSpeed;
  double    peerAvgSpeed;

  int32_t	cdnRecvLen;
  int32_t	peerRecvLen;
  bool	    complete;		//是否下载完毕
  int32_t   type;			//0:普通下载 1:跳转 
  double   play_start;     //播放开始时间
  int32_t   duration;       //文件播放时长
  int32_t   byterate;       //文件比特率
  int32_t   state_code;      //1:P2P 2:HTTP 3:P2P|HTTP 4:START 5:START|P2P 6:START|HTTP
  int32_t   max_con;        //最大连接数

  bool       free_cdn; 
  SHDownloadStat()
  {
    memset(this,0,sizeof(SHDownloadStat));
    free_cdn = true;
  }
};
//
typedef struct tagSHPeerPunchInfo
{
    uint8_t             natType;
    uint16_t            sucessNum;
    uint16_t            failedNum;
    tagSHPeerPunchInfo()
    {
        natType		= 0;
        sucessNum   = 0;
        failedNum   = 0;
    }
}SHPeerPunchInfo;
//
typedef struct tagSHPeerReportInfo
{
    int32_t				peerId;
    int32_t			    avgSpeed;
    SHPeerStatus	status;
    int32_t				mapipTime;//获取外网地址时间
    int32_t             fileRequestTime;//文件交换时间
    int32_t             punchTime;//打洞时间

    tagSHPeerReportInfo()
    {
        peerId	=	0;
        avgSpeed =	0;
        status	=	SHPeerStatus_Closed;
        mapipTime	=	-1;
        fileRequestTime	=	-1;
        punchTime=	-1;
    }
}SHPeerReportInfo;
//
typedef struct tagSHStatDownloadInfo
{
    int16_t							avgCdnSpeed; //cdn  KBbs
    int16_t							avgPeerSpeed;//peer KBbs
    uint32_t						p2pByte;
    uint32_t						cdnByte;
    SHDType						shdtype;
    int32_t							liveid;
    int32_t                         videoid;
    int32_t                         sectionNum;
    int32_t                         fetchPeerTime;
    int32_t                         freecdn;
    vector<SHPeerPunchInfo>		peersPunchInfo;
    vector<SHPeerReportInfo>      peerReportInfo;
    tagSHStatDownloadInfo()
    {
        avgCdnSpeed		= 0;
        avgPeerSpeed	= 0;
        p2pByte			= 0;
        cdnByte			= 0;
        shdtype			= (SHDType)-1;
        liveid			= -1;
        fetchPeerTime   = -1;
        videoid         = 0;
        sectionNum      = 0;
        freecdn         = 1;
    }
}SHStatDownloadInfo;

//下载信息保存节点
typedef struct tagSHReportDownloadInfo
{
    SHStatDownloadInfo			downloadInfo;
    std::tstring				file_name;
    int32_t						num;
    int32_t						vid ;
    tagSHReportDownloadInfo()
    {
        num=0;
        vid=0;
    }
}SHReportDownloadInfo;


//测速类型
enum TEST_SPEED_TYPE
{
	TEST_SPEED_NONE = 0,
	TEST_SPEED_SERVER,			 //服务器下发测速
	TEST_SPEED_USER,			 //用户主动测速
	TEST_SPEED_DOWNLOADER,		 //下载测速
	TEST_SPEED_BUFFER			 //发生缓冲时用户选择测速
};

enum EServerTestSpeedError
{
	EServerTestSpeed_Undefined = -1,
	EServerTestSpeed_NoError= 0,
	EServerTestSpeed_DispatchError,
	EServerTestSpeed_RequestError,				//HTTP错误
	EServerTestSpeed_HasLivePlayTask,				//当前有直播任务
	EServerTestSpeed_Existed,						//该文件已存在
	EServerTestSpeed_IsDownloading,				//该任务正在执行
	EServerTestSpeed_Expired,						//开始执行时的时间---收到时的时间大于规定值
	EServerTestSpeed_Ternimnate,					//服务端中止
	EServerTestSpeed_ExceedAllowDownloadType,    	//超出允许的下载时间
	EServerTestSpeed_ExceedNumLimit,			   //超出允许的下载个数
	EServerTestSpeed_NotAllowedRequest              //不允许请求的段号，目前只允许请求第一段
};
#endif
