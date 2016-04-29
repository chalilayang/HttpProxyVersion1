
#include "udp_def.h"
#include "porting_event.h"

class ISHPunchInterface
{
public:
    ISHPunchInterface(){};
    virtual ~ISHPunchInterface(){};
    virtual bool punch(const SHPunchParam& param, porting::EventHandle hEvent) = 0;
};

class CSHPunch
    :public boost::noncopyable
    ,public boost::enable_shared_from_this<CSHPunch>
{
public:
	CSHPunch(void);
	~CSHPunch(void);
public:
	bool   punch(const SHPunchParam& param, int32_t& punchSucceedIp, int16_t& punchSucceedPort,int32_t& punch_use_time);
private:
	ISHPunchInterface *get_punch_obj(bool sponsor, SHNatType selfNatType, SHNatType peerNatType);
	void   recv_proc();

private:
    porting::EventHandle punch_succeed_event_;
	SOCKET	punch_socket_;
	int16_t	    peer_port_;
	int32_t		peer_ip_;
	bool	is_running_;
};


