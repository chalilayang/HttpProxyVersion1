#ifndef __SH_LOCAL_SERVER_API_H__
#define __SH_LOCAL_SERVER_API_H__

#include "../localhttpserver/localserver.h"
#include "sh_p2p_system_define.h"

#ifdef __cplusplus
extern "C" {
#endif 

	//init local server
	LOCALSERVER_API bool LOCALSERVER_CALL init_local_server(SHP2PSystemParam sys_param); 

	//uninit local server
	LOCALSERVER_API bool LOCALSERVER_CALL uninit_local_server(void);

	//stop play (only play)
	LOCALSERVER_API void LOCALSERVER_CALL stop_play_vid(int vid, bool ismytv, int definition);

	//stop download (only download)
	LOCALSERVER_API void LOCALSERVER_CALL stop_download_vid(int vid, bool ismytv, int definition);

    LOCALSERVER_API void LOCALSERVER_CALL set_allow_share(bool allow_share);
#ifdef __cplusplus
}
#endif

#endif //__SH_LOCAL_SERVER_API_H__