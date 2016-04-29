#ifndef __MACRO_DEFINE_H__
#define __MACRO_DEFINE_H__

#include "sh_p2p_system_define.h"

enum SHDOWNLOAD_TYPE
{
	SHDOWNLOAD_NONE = 0,
	SHDOWNLOAD_PLAY,    //���߲���
	SHDOWNLOAD_OFFLINE,  //��������
	SHDOWNLOAD_FOXPLAY  //����
};

typedef struct tagHttpRequestInfo
{
	std::string type;
	int32_t vid;
	int32_t dnum;
	int32_t pnum;
	int32_t startpos;
	int32_t size;
	bool isMy;
	bool isclose;
	int definition;
	sh_int_64 unique_id;
	sh_int_64 call_id;

	tagHttpRequestInfo()
	{
		init();
	}

	void init()
	{
		vid = 0;
		dnum = 0;
		pnum = 0;
		startpos = 0;
		size = 0;
		isMy = false;
		isclose = false;
		definition = 21; //Ĭ�ϳ���
		unique_id = 0;
		call_id = 0;
	}
}HttpRequestInfo; 

typedef struct tagDOWNLOADVIDEOINFO
{
	int32_t vid;
	bool ismytv;
	int32_t definition;
	std::tstring name;
	std::tstring cache_path;
	int32_t download_pos;
	sh_uint_32 state;
	double percent;
	double speed;	

	tagDOWNLOADVIDEOINFO()
	{
		memset(this, 0, sizeof(tagDOWNLOADVIDEOINFO));
	}

}DOWNLOADVIDEOINFO;

//�߳̿���
enum LocalServerThread
{
	K_THREAD_LISTEN = 0, //�����߳�
	K_THREAD_SEND,	     //��������ͷ��������߳�
	K_THREAD_SUM
};

//#define K_THREAD_LISTEN 0
//#define K_THREAD_SEND K_THREAD_LISTEN
//#define K_THREAD_SUM 1

#define LOCAL_SERVER_BUFFER_SIZE  200 //(200*16K)


#ifndef WIN32
#define _atoi64(val) strtoll(val, NULL, 10)
#endif


#endif //__MACRO_DEFINE_H__
