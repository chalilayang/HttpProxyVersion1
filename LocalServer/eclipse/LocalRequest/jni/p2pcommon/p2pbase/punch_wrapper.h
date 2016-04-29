#ifndef    PUNCH_WRAPPER_H_
#define  PUNCH_WRAPPER_H_

#include "../base/common.h"
#include "udp_def.h"

class PunchWrapper
    : private boost::noncopyable 
    , public boost::enable_shared_from_this<PunchWrapper>
{
public:
    static boost::shared_ptr<PunchWrapper> inst()
    {
        boost::call_once(PunchWrapper::init_it, s_once_flag_);
        return s_pinst_;
    }
    //打洞并连接，通过 SH_PUNCH_CALLBACK 回调结果
    void punch_and_connect(const SHPunchParam& param, const SH_PUNCH_CALLBACK& callback);
    static void thread_proc(const SHPunchParam& param, const SH_PUNCH_CALLBACK& callback);

private:
    static void init_it()
    {
        s_pinst_.reset(new PunchWrapper);
    }

    static boost::shared_ptr<PunchWrapper> s_pinst_;
    static boost::once_flag s_once_flag_;
};

#endif