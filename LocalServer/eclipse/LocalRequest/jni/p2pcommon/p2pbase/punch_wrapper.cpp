#include "punch_wrapper.h"
#include "punch.h"
#include "../base/threadpool.hpp"
#include "../sh_kernel.h"

using namespace boost::threadpool;

boost::shared_ptr<PunchWrapper> PunchWrapper::s_pinst_;
boost::once_flag PunchWrapper::s_once_flag_ = BOOST_ONCE_INIT;

void PunchWrapper::punch_and_connect( const SHPunchParam& param, const SH_PUNCH_CALLBACK& callback )
{
	SHKernel::instance()->shedule_block_punch(param,callback);
}

void PunchWrapper::thread_proc( const SHPunchParam& param, const SH_PUNCH_CALLBACK& callback )
{
    boost::shared_ptr<CSHPunch>  punchObj(new CSHPunch);
    int32_t peer_ip = param.nPeerMapIp;
    int16_t peer_port = param.nPeerLocalPort;
	int32_t punch_use_time = 0;
    //开始打洞
    bool result = punchObj->punch(param, peer_ip, peer_port,punch_use_time);
    if(callback)
    {
        callback((result ? SHPunchConnect_PunchSucceed : SHPunchConnect_PunchFailed), peer_ip, peer_port,punch_use_time);
    }
}