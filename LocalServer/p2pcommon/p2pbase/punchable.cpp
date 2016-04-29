#include "punchable.h"
#include "../sh_kernel.h"
#include "../log/log.h"
#include "../base/socket_api.h"
#include "../base/algorithm.h"
#include "async_punch_obj.h"
#include "async_stun_obj.h"

uint32_t Punchable::stun_ip_ = 0;
uint16_t Punchable::stun_port_ = 0;
Punchable::Punchable(boost::asio::io_service &ios):
ios_(ios),
native_socket_(INVALID_SOCKET)
{

}

Punchable::~Punchable()
{

}

Punchable::Ptr Punchable::create(boost::asio::io_service &ios)
{
	return Ptr(new Punchable(ios));
}

bool Punchable::get_mapped_address(SOCKET native_socket,const SH_MAPIP_CALLBACK &callback)
{
	bool ret = true;
	do 
	{		
		ios_.post(boost::bind(&Punchable::binding,shared_from_this(),native_socket,callback));
	} while (0);
	return ret;
}

bool Punchable::punch_and_connect(const SHPunchParam &param,const SH_PUNCH_CALLBACK &callback)
{
	bool ret = true;
	do 
	{
		ios_.post(boost::bind(&Punchable::punch,shared_from_this(),param,callback));
	} while (0);
	return ret;
}

bool Punchable::binding(SOCKET native_socket,const SH_MAPIP_CALLBACK &callback)
{
	bool ret = true;
	do 
	{
		//Check param.
		if (native_socket == INVALID_SOCKET)
		{
			ERROR_LOG("udptrace",_T("Invalid socket fd\n"));
			ret = false;
			break;
		}

		//Check param.
		if (callback.empty())
		{
			ERROR_LOG("udptrace",_T("[%d] Invalid callback\n"),native_socket);
			ret = false;
			break;
		}

		//Clear state first.
		stop();

		//Do assign.
		native_socket_ = native_socket;
		asio_socket_ = SocketPtr(new boost::asio::ip::udp::socket(ios_));
		asio_socket_->assign(boost::asio::ip::udp::v4(),native_socket_);

		//Do binding.
		binding_obj_ = create_binding_obj(callback);
		ret = binding_obj_->binding();
	} while (0);

	if (!ret && !callback.empty())
	{
		callback(native_socket,0,0);
	}
	return ret;
}

bool Punchable::punch(const SHPunchParam &param,const SH_PUNCH_CALLBACK &callback)
{
	bool ret = true;
	do 
	{
		//Check param.
		if (callback.empty())
		{
			ERROR_LOG("udptrace",_T("[%d] Invalid callback\n"),native_socket_);
			ret = false;
			break;
		}

		//Check socket.
		if (asio_socket_ == NULL)
		{
			ERROR_LOG("udptrace",_T("[%d] Invalid socket obj\n"),native_socket_);
			ret = false;
			break;
		}

		//Check socket.
		if (!asio_socket_->is_open())
		{
			ERROR_LOG("udptrace",_T("[%d] Invalid socket state\n"),native_socket_);
			ret = false;
			break;
		}

		//Check punch_obj_.
		if (punch_obj_ != NULL)
		{
			punch_obj_->stop();
			punch_obj_.reset();
		}

		//Do punch.
		punch_obj_ = create_punch_obj(param,callback);
		if (punch_obj_ == NULL)
		{
			ERROR_LOG("udptrace",_T("[%d] Invalid punch obj\n"),native_socket_);
			ret = false;
			break;
		}

		ret = punch_obj_->punch();
	} while (0);

	if (!ret && !callback.empty())
	{
		callback(SHPunchConnect_PunchFailed,param.nPeerMapIp,param.nPeerMapPort,0);
	}
	return ret;
}

Punchable::PunchObjPtr Punchable::create_punch_obj(
	const SHPunchParam &param,
	const SH_PUNCH_CALLBACK &callback)
{
	PunchObjPtr punch_obj;
	do 
	{
		DEBUG_LOG("udptrace", _T("sponsor = %d,selfNatType = %d,peerNatType = %d\n"), param.sponsor,param.selfNatType,param.peerNatType);
		if(param.selfNatType == NatType_ERROR || param.peerNatType == NatType_ERROR)
		{
			DEBUG_LOG("udptrace", _T("One natType error,can't punch,GetPunchObj failed\n"));
			break;
		}

		if(param.selfNatType == NatType_BLOCKED || param.peerNatType == NatType_BLOCKED)
		{
			DEBUG_LOG("udptrace", _T("One natType is blocked,can't punch,GetPunchObj failed\n"));
			break;
		}

		if((param.selfNatType == NatType_FULL_CONE || param.selfNatType == NatType_OPEN_INTERNET) && 
			(param.peerNatType == NatType_FULL_CONE || param.peerNatType == NatType_OPEN_INTERNET))
		{
			if(param.sponsor)
			{
				DEBUG_LOG("udptrace", _T("self/peer nattype is open or fullcone and self is sponsor,select punch obj is AsyncPunchImp2\n"));
				punch_obj = AsyncPunchImp2::create(
					ios_,
					asio_socket_,
					param,
					callback);
				break;
			}

			DEBUG_LOG("udptrace", _T("self/peer natType is open or fullcone and self is not sponsor,select punch obj is AsyncPunchImp1\n"));
			punch_obj = AsyncPunchImp1::create(
				ios_,
				asio_socket_,
				param,
				callback);
			break;
		}

		if(param.selfNatType == NatType_FULL_CONE || param.selfNatType == NatType_OPEN_INTERNET)
		{
			DEBUG_LOG("udptrace", _T("self natType is open or fullcone ,select punch obj is AsyncPunchImp1\n"));
			punch_obj = AsyncPunchImp1::create(
				ios_,
				asio_socket_,
				param,
				callback);
			break;
		}

		if(param.peerNatType == NatType_FULL_CONE || param.peerNatType == NatType_OPEN_INTERNET)
		{
			DEBUG_LOG("udptrace",  _T("peer natType is open or fullcone ,select punch obj is AsyncPunchImp2\n"));
			punch_obj = AsyncPunchImp2::create(
				ios_,
				asio_socket_,
				param,
				callback);
			break;
		}

		if(param.peerNatType == NatType_SYMMETRIC_NAT)
		{
			DEBUG_LOG("udptrace", _T("peer natType is SYMMETRIC ,select punch obj is AsyncPunchImp3\n"));
			punch_obj = AsyncPunchImp3::create(
				ios_,
				asio_socket_,
				param,
				callback);
			break;
		}

		DEBUG_LOG("udptrace", _T("select punch obj is AsyncPunchImp2\n"));
		punch_obj = AsyncPunchImp2::create(
			ios_,
			asio_socket_,
			param,
			callback);
	} while (0);
	return punch_obj;
}

Punchable::BindingPtr Punchable::create_binding_obj(const SH_MAPIP_CALLBACK &callback)
{
    if (stun_ip_ == 0 || stun_port_ == 0)
    {
        return BindingPtr(new AsyncBindingObj(
            ios_,
            asio_socket_,
            callback));
    }
    else
    {
        return BindingPtr(new AsyncBindingObj(
            ios_,
            asio_socket_,
            callback,
            0,
            false,
            false,
            stun_ip_,
            stun_port_));
    }
}

bool Punchable::close_socket()
{
	bool ret = true;
	do 
	{
		if (asio_socket_ != NULL)
		{
			if (asio_socket_->is_open())
			{
				asio_socket_->close();
			}
			asio_socket_.reset();
		}
		else if (native_socket_ != INVALID_SOCKET)
		{
			SocketAPI::closesocket_ex(native_socket_);
		}
		native_socket_ = INVALID_SOCKET;
	} while (0);
	return ret;
}

bool Punchable::stop()
{
	bool ret = true;
	do 
	{
		if (binding_obj_ != NULL)
		{
			binding_obj_->stop();
			binding_obj_.reset();
		}

		if (punch_obj_ != NULL)
		{
			punch_obj_->stop();
			punch_obj_.reset();
		}

		close_socket();
	} while (0);
	return ret;
}

void Punchable::set_stun_address(uint32_t ip,uint16_t port)
{
    stun_ip_ = ip;
    stun_port_ = port;
}

