#include "../include/sh_local_server_api.h"
#include "../p2pcommon/log/log.h"
#include "local_http_server.h"

bool LOCALSERVER_CALL init_local_server(SHP2PSystemParam sys_param)
{
	return LocalHttpServer::Inst()->start(sys_param);
}

bool LOCALSERVER_CALL uninit_local_server(void)
{
	LocalHttpServer::Inst()->stop();
	return true;
}

void LOCALSERVER_CALL stop_play_vid(int vid, bool ismytv, int definition)
{
	INFO_LOG(LOCAL_SERVER_LOG,_T("called stop_play_vid() vid=%d, ismytv=%d, definition=%d\n"), vid, ismytv, definition);

	sh_int_64 unique_id = ((sh_int_64)ismytv) << 32 | ((sh_int_64) definition) << 33 | ((sh_int_64) kSHRequest_Play) << 38 | vid;
	//LocalHttpServer::Inst()->stop_request_video_data(unique_id);

	INFO_LOG(LOCAL_SERVER_LOG,_T("called stop_play_vid() 2 vid=%d, ismytv=%d, definition=%d\n"), vid, ismytv, definition);

}

void LOCALSERVER_CALL stop_download_vid(int vid, bool ismytv, int definition)
{
	INFO_LOG(LOCAL_SERVER_LOG,_T("called stop_download_vid() vid=%d, ismytv=%d, definition=%d\n"), vid, ismytv, definition);

	sh_int_64 unique_id = ((sh_int_64)ismytv) << 32 | ((sh_int_64) definition) << 33 | ((sh_int_64) kSHRequest_Download) << 38 | vid;
	LocalHttpServer::Inst()->stop_request_video_data(unique_id);
}

LOCALSERVER_API void LOCALSERVER_CALL set_allow_share( bool allow_share )
{
    INFO_LOG(LOCAL_SERVER_LOG, _T("set allow share\n"));
    LocalHttpServer::Inst()->set_allow_share(allow_share);
}