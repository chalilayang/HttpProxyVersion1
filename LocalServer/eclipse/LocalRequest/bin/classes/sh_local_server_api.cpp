#include "../include/sh_local_server_api.h"
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
