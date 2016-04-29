#ifndef __I_P2P_LOCALSERVER_H__
#define __I_P2P_LOCALSERVER_H__

//p2p��localserver�����ӿ�
#include "sh_p2p_system_define.h"


class IP2PDownloadSink;
class IP2PDownload
{
public:
	IP2PDownload() {}
	virtual ~IP2PDownload() {}

	//�ͷ�
	virtual void release(void) = 0;

	//ͨ��pSink�ص���localserver
	virtual void advice(IP2PDownloadSink *pSink) = 0; 
	virtual void un_advice(IP2PDownloadSink *pSink) = 0;

	//��ʼ��p2pϵͳ
	virtual bool init_p2p_system(SHP2PSystemParam system_param) = 0; 

	//����ʼ��p2pϵͳ
	virtual bool uninit_p2p_system(void) = 0;

	//�ֶ�����mp4����(����)
	virtual sh_int_64 start_request_video_data(sh_int_32 vid, enum SHVideoClarity clarity, sh_int_32 index, bool is_mytv, sh_int_32 pnum) = 0;

	//ֹͣ����mp4����
	virtual void stop_request_video_data(sh_int_64 unique_id) = 0;

	//��ͣ����
	virtual void pause_request_video_data(sh_int_64 unique_id) = 0;

	//�ϵ�����
	virtual void restart_request_video_data(sh_int_64 unique_id) = 0;

	//range����(����)
	virtual sh_int_64 start_request_video_data_range_ott(sh_int_32 vid, enum SHVideoClarity clarity, bool is_mytv, enum SHRequestType dl_type, sh_int_32 start_range, sh_int_32 end_range, sh_int_64 call_id) = 0;

	//��������
	virtual sh_int_64 start_download_video_data(sh_int_32 vid, enum SHVideoClarity clarity, sh_int_32 index, bool is_mytv, sh_int_32 start_range, sh_int_32 end_range) = 0;

	//��ȡ����������Ϣ
	virtual bool get_download_info(NewSHDispInfo* sh_disp_info) = 0;

    //�����Ƿ��ϴ�
    virtual void set_allow_share(bool allow_share) = 0;
};

class IP2PDownloadSink
{
public:
	IP2PDownloadSink() {}
	virtual ~IP2PDownloadSink() {}

	//p2p�쳣����
	virtual void on_p2p_happen_error(sh_int_64 unique_id, sh_uint_32 index, enum SHP2pErrorType error_type, sh_uint_32 error_code, sh_uint_32 status_code) = 0;
	
	//����p2p����
	virtual void on_recv_p2p_video_data(sh_int_64 unique_id, sh_uint_32 index, sh_int_8_p data, sh_uint_32 len, bool isheader, sh_int_32 real_mp4_size, sh_int_32 cur_download_pos, int64_t call_id) = 0;
	
	//����mp4�������
	virtual void on_finish_p2p_video_data(sh_int_64 unique_id) = 0;

    //ȡ��ע��ID
	virtual void on_get_register_id(sh_int_32 id, sh_char_p sohu_key) = 0;
};

#endif //__I_P2P_LOCALSERVER_H__