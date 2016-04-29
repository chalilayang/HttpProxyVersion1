#ifndef __PUNCHABLE_H__
#define __PUNCHABLE_H__

#include "../base/common.h"
#include "../p2pbase/udp_def.h"

class AsyncBindingObj;
class AsyncPunchObjBase;
class Punchable : public boost::enable_shared_from_this<Punchable>
{
public:
	typedef boost::shared_ptr<boost::asio::ip::udp::socket> SocketPtr;
	typedef boost::shared_ptr<AsyncBindingObj> BindingPtr;
	typedef boost::shared_ptr<AsyncPunchObjBase> PunchObjPtr;
	typedef boost::shared_ptr<Punchable> Ptr;
	static Ptr create(boost::asio::io_service &ios);
	~Punchable();

public:
	bool get_mapped_address(SOCKET native_socket,const SH_MAPIP_CALLBACK &callback);
	bool punch_and_connect(const SHPunchParam &param,const SH_PUNCH_CALLBACK &callback);
	bool stop();
	SocketPtr get_socket(){return asio_socket_;}
    static void set_stun_address(uint32_t ip,uint16_t port);
	
private:
	Punchable(boost::asio::io_service &ios);
	bool binding(SOCKET native_socket,const SH_MAPIP_CALLBACK &callback);
	bool punch(const SHPunchParam &param,const SH_PUNCH_CALLBACK &callback);
	PunchObjPtr create_punch_obj(const SHPunchParam &param,const SH_PUNCH_CALLBACK &callback);
	BindingPtr create_binding_obj(const SH_MAPIP_CALLBACK &callback);
	bool close_socket();

private:
	boost::asio::io_service &ios_;
	SOCKET native_socket_;
	SocketPtr asio_socket_;
	BindingPtr binding_obj_;
	PunchObjPtr punch_obj_;
    static uint32_t stun_ip_;
    static uint16_t stun_port_;
};

#endif
