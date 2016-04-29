#include "async_punch_obj.h"
#include "punch_protocol_imp.h"
#include "../log/log.h"
#include "../base/algorithm.h"

#define BOOST_ASIO_DISABLE_IOCP 1

////////////////////////////////////AsyncPunchObjBase//////////////////////////////////////
AsyncPunchObjBase::AsyncPunchObjBase(
    boost::asio::io_service &ios,
	boost::shared_ptr<boost::asio::ip::udp::socket> socket_ptr,
    const SHPunchParam &param,
    const SH_PUNCH_CALLBACK callback):
	socket_(socket_ptr),
    punch_param_(param),
    callback_(callback),
    punch_timer_(AsyncWaitTimer::create(ios)),
    timeout_counter_(0),
    packet_count_per_time_(kPacketCountSendPerTime),
    rcv_flag_(false)
{
    //DEBUG_LOG("udptrace",_T("%s[%x] AsyncPunchObjBase created\n"),b2w(to_string()).c_str(),this);
}

AsyncPunchObjBase::~AsyncPunchObjBase()
{
    //DEBUG_LOG("udptrace",_T("%s[%x] AsyncPunchObjBase destroyed\n"),b2w(to_string()).c_str(),this);
}

bool AsyncPunchObjBase::punch()
{
    bool ret = true;
    do 
    {
        DEBUG_LOG(
            "udptrace", 
            _T("%s Punching peer\n"),
            b2w(to_string()).c_str());
        
        //Dispatch receive operation to boost asio io_service
        async_rcv();
        
        //Do actual punch operation under different NAT type.
        active();
    } while (0);

    return ret;
}

bool AsyncPunchObjBase::stop()
{
    bool ret = true;
    do 
    {       
        if (punch_timer_)
        {
            punch_timer_->cancel();
            punch_timer_.reset();
        }
    } while (0);
    return ret;
}

bool AsyncPunchObjBase::send_packet(SHUdpCmdType packet_type,uint32_t target_ip,uint32_t target_port)
{
    bool ret = true;
    do 
    {
        if (socket_ == NULL)
        {
            ret = false;
            break;
        }

        if (!socket_->is_open())
        {
            ret = false;
            break;
        }

        char buffer[64] = {0}; //No more then 9 bytes when punching.
        int length = sizeof(buffer);

        ret = PunchProtocolImp::encode_punch(packet_type,buffer,length);
        if (!ret)
        {
            ret  = false;
            break;
        }

        boost::asio::ip::udp::endpoint target_endpoint(boost::asio::ip::address::from_string(uint2ip(target_ip)),ntohs(target_port));
        DEBUG_LOG(
            "udptrace",
            _T("%s Send punch packet to %s:%d type:%d\n"),
            b2w(to_string()).c_str(),
            b2w(uint2ip(target_ip)).c_str(),
            ntohs(target_port),
            packet_type);
        for (int i = 0;i < packet_count_per_time_;++i)
        {
            socket_->async_send_to(
                boost::asio::buffer(buffer,length),
                target_endpoint,
                boost::bind(
                    &AsyncPunchObjBase::handle_send_packet,
                    shared_from_this(),
                    boost::asio::placeholders::error,
                    boost::asio::placeholders::bytes_transferred));
        }
    } while (0);
    return ret;
}

bool AsyncPunchObjBase::reply_packet(SHUdpCmdType packet_type)
{
    bool ret = true;
    do 
    {
        if (socket_ == NULL)
        {
            ret = false;
            break;
        }

        if (!socket_->is_open())
        {
            ret = false;
            break;
        }

        char buffer[64] = {0};
        int length = sizeof(buffer);

        ret = PunchProtocolImp::encode_punch(packet_type,buffer,length);
        if (!ret)
        {
            ret  = false;
            break;
        }

        for (int i = 0;i < packet_count_per_time_;++i)
        {
            socket_->async_send_to(
                boost::asio::buffer(buffer,length),
                rcv_end_point_,
                boost::bind(
                    &AsyncPunchObjBase::handle_send_packet,
                    shared_from_this(),
                    boost::asio::placeholders::error,
                    boost::asio::placeholders::bytes_transferred));
        }
    } while (0);
    return ret;
}

bool AsyncPunchObjBase::handle_send_packet(const boost::system::error_code &error,std::size_t bytes_transferred)
{
    bool ret = true;
    do 
    {

    } while (0);
    return ret;
}

std::string AsyncPunchObjBase::to_string()
{
    do 
    {
        if (!punch_info_.empty())
        {
            break;
        }

        char info[1024] = {0};
        snprintf(
            info,
            sizeof(info),
            "[Punching][(%s:%u)-(%s:%u)-(%d:%d)-(%d)]",
            uint2ip(punch_param_.nPeerMapIp).c_str(), 
            ntohs(punch_param_.nPeerMapPort),
            uint2ip(punch_param_.nPeerLocalIp).c_str(), 
            ntohs(punch_param_.nPeerLocalPort),
            punch_param_.selfNatType,
            punch_param_.peerNatType,
            punch_param_.sock);
        punch_info_ = info;
    } while (0);

    return punch_info_;
}

void AsyncPunchObjBase::active()
{
    spend_time_.restart();
	if (punch_timer_ != NULL)
	{
		punch_timer_->set_wait_millseconds(SH_UDP_PUNCH_TIMEOUT/SH_PUNCH_RETRY_NUM);
		punch_timer_->set_wait_times(SH_PUNCH_RETRY_NUM + 1);
		punch_timer_->async_wait(boost::bind(&AsyncPunchObjBase::handle_timeout,shared_from_this()));
	}
}

bool AsyncPunchObjBase::async_rcv()
{
    bool ret = true;
    do 
    {
        if (socket_ == NULL)
        {
            ret = false;
            break;
        }

        if (!socket_->is_open())
        {
            ret = false;
            break;
        }

        //Control sequence receive operation,don't mess up with the receive buffer.
        if (rcv_flag_)
        {
            break;
        }

        rcv_flag_ = true;
        reset_buffer();
        socket_->async_receive_from(
            boost::asio::buffer(rcv_buffer_,sizeof(rcv_buffer_)),
            rcv_end_point_,
            boost::bind(
            &AsyncPunchObjBase::handle_rcv_packet,
            shared_from_this(),
            boost::asio::placeholders::error,
            boost::asio::placeholders::bytes_transferred));
    } while (0);
    return ret;
}

void AsyncPunchObjBase::reset_buffer()
{
    memset(rcv_buffer_,0,sizeof(rcv_buffer_));
}

bool AsyncPunchObjBase::handle_rcv_packet(const boost::system::error_code &error,std::size_t bytes_transferred)
{
    bool ret = true;
    do 
    {
        rcv_flag_ = false;
        if (error || bytes_transferred == 0)
        {
#ifdef WIN32
            if (error.value() == WSAEWOULDBLOCK || error.value() == WSAECONNRESET || error.value() == WSAECONNREFUSED)
#else
            if (error.value() == EWOULDBLOCK || error.value() == ECONNRESET || error.value() == ECONNREFUSED)
#endif
            {
                async_rcv();
            }
            
            ERROR_LOG("udptrace",_T("%s receive errno:%d,bytes_transferred:%d\n"),b2w(to_string()).c_str(),error.value(),bytes_transferred);
            break;
        }

        DEBUG_LOG(
            "udptrace",
            _T("%s Receive %d byte from %s:%u\n"),
            b2w(to_string()).c_str(),
            bytes_transferred,
            b2w(rcv_end_point_.address().to_string()).c_str(),
            rcv_end_point_.port());
        SHUdpHeader packet;
        ret = PunchProtocolImp::decode_punch(rcv_buffer_,bytes_transferred,packet);
        if (!ret)
        {
            async_rcv(); //Not punch message,receive more.
            ret = false;
            break;
        }

        switch(packet.cmd)
        {
        case SHUdpCmd_Punch:
            ret = handle_punch_request();
            break;
        case SHUdpCmd_Punch_Rsp:
            ret = handle_punch_response();
            break;
        case SHUdpCmd_Punch_Rsp_Rsp:
            ret = handle_punch_response_response();
            break;
        default:
            async_rcv(); //Not punch message,receive more.
            break;
        }
    } while (0);

    return ret;
}

bool AsyncPunchObjBase::handle_punch_request()
{
    bool ret = true;
    do 
    {
        ret = reply_packet(SHUdpCmd_Punch_Rsp);
        if (!ret)
        {
            break;
        }

        //Waiting for RSP_RSP
        async_rcv();
    } while (0);
    return ret;
}

bool AsyncPunchObjBase::handle_punch_response()
{
    bool ret = true;
    do 
    {
        ret = reply_packet(SHUdpCmd_Punch_Rsp_Rsp);
        if (!ret)
        {
            break;
        }

        on_punch_success();
    } while (0);
    return ret;
}

bool AsyncPunchObjBase::handle_punch_response_response()
{
    bool ret = true;
    do 
    {
        on_punch_success();
    } while (0);
    return ret;
}

bool AsyncPunchObjBase::handle_timeout()
{
    bool ret = true;
    do 
    {        
        timeout_counter_++;
        if (timeout_counter_ > 1)
        {
            DEBUG_LOG("udptrace",_T("%s timeout for %dth time\n"),b2w(to_string()).c_str(),timeout_counter_ - 1);
        }

        if (timeout_counter_ <= SH_PUNCH_RETRY_NUM)
        {
            ret = do_punch();
            break;
        }

        ret = on_punch_failed();
    } while (0);
    return ret;
}

bool AsyncPunchObjBase::on_punch_failed()
{
    bool ret = true;
    do 
    {
        if (punch_timer_)
        {
            punch_timer_->cancel();
            punch_timer_.reset();
        }

        int32_t spend_ms = (int32_t)spend_time_.elapsed();

        DEBUG_LOG(
            "udptrace", 
            _T("%s Punch peer failed,spent time:%dms\n"),
            b2w(to_string()).c_str(),
            spend_ms);

        if (callback_ != NULL)
        {
            callback_(SHPunchConnect_PunchFailed,punch_param_.nPeerMapIp,punch_param_.nPeerMapPort,spend_ms);
        }
    } while (0);

    return ret;
}

bool AsyncPunchObjBase::on_punch_success()
{
    bool ret = true;
    do 
    {       
        if (punch_timer_)
        {
            punch_timer_->cancel();
            punch_timer_.reset();
        }
        
        int32_t spend_ms = (int32_t)spend_time_.elapsed();

        DEBUG_LOG(
            "udptrace", 
            _T("%s Punch peer success,spent time:%dms\n"),
            b2w(to_string()).c_str(),
            spend_ms);

        if (callback_ != NULL)
        {
            callback_(
                SHPunchConnect_PunchSucceed,
                htonl(rcv_end_point_.address().to_v4().to_ulong()),
                htons(rcv_end_point_.port()),
                spend_ms);
        }
    } while (0);

    return ret;
}

void AsyncPunchObjBase::print_errno(const std::string &function_name)
{
#ifdef WIN32
    ERROR_LOG(
        "udptrace",
        _T("%s %s errno:%d\n"),
        b2w(to_string()).c_str(),
        b2w(function_name).c_str(),
        WSAGetLastError());
#else
    ERROR_LOG(
        "udptrace",
        _T("%s %s errno:%d\n"),
        b2w(to_string()).c_str(),
        b2w(function_name).c_str(),
        errno);
#endif
}

/////////////////////////////////////AsyncPunchImp1/////////////////////////////////////
AsyncPunchImp1::AsyncPunchImp1(
    boost::asio::io_service &ios,
	boost::shared_ptr<boost::asio::ip::udp::socket> socket_ptr,
    const SHPunchParam &param,
    const SH_PUNCH_CALLBACK callback):
    AsyncPunchObjBase(ios,socket_ptr,param,callback)
{

}

AsyncPunchImp1::~AsyncPunchImp1()
{

}

AsyncPunchImp1::Ptr AsyncPunchImp1::create(
    boost::asio::io_service &ios,
	boost::shared_ptr<boost::asio::ip::udp::socket> socket_ptr,
    const SHPunchParam &param,
    const SH_PUNCH_CALLBACK callback)
{
    return Ptr(new AsyncPunchImp1(ios,socket_ptr,param,callback));
}

bool AsyncPunchImp1::do_punch()
{
    bool ret = true;
    do 
    {
        //Do nothing,just wait as client located in full cone NAT or open internet.
    } while (0);
    return ret;
}

/////////////////////////////////////AsyncPunchImp2/////////////////////////////////////
AsyncPunchImp2::AsyncPunchImp2(
    boost::asio::io_service &ios,
	boost::shared_ptr<boost::asio::ip::udp::socket> socket_ptr,
    const SHPunchParam &param,
    const SH_PUNCH_CALLBACK callback):
    AsyncPunchObjBase(ios,socket_ptr,param,callback)
{

}

AsyncPunchImp2::~AsyncPunchImp2()
{

}

AsyncPunchImp2::Ptr AsyncPunchImp2::create(
    boost::asio::io_service &ios,
	boost::shared_ptr<boost::asio::ip::udp::socket> socket_ptr,
    const SHPunchParam &param,
    const SH_PUNCH_CALLBACK callback)
{
    return Ptr(new AsyncPunchImp2(ios,socket_ptr,param,callback));
}

bool AsyncPunchImp2::do_punch()
{
    bool ret = true;
    do 
    {
        //Send punch request to remote peer's local address.
        ret = send_packet(
            SHUdpCmd_Punch,
            punch_param_.nPeerLocalIp,
            punch_param_.nPeerLocalPort);
        if (!ret)
        {
            break;
        }

        //Check if remote peer is in open internet.
        if (punch_param_.nPeerLocalIp == punch_param_.nPeerMapIp && 
            punch_param_.nPeerLocalPort == punch_param_.nPeerMapPort)
        {
            break;
        }

        //If not,send punch request to remote peer's mapped address.
        ret = send_packet(
            SHUdpCmd_Punch,
            punch_param_.nPeerMapIp,
            punch_param_.nPeerMapPort);
    } while (0);
    return ret;
}

/////////////////////////////////////AsyncPunchImp3/////////////////////////////////////
AsyncPunchImp3::AsyncPunchImp3(
    boost::asio::io_service &ios,
	boost::shared_ptr<boost::asio::ip::udp::socket> socket_ptr,
    const SHPunchParam &param,
    const SH_PUNCH_CALLBACK callback):
    AsyncPunchObjBase(ios,socket_ptr,param,callback)
{

}

AsyncPunchImp3::~AsyncPunchImp3()
{

}

AsyncPunchImp3::Ptr AsyncPunchImp3::create(
    boost::asio::io_service &ios,
	boost::shared_ptr<boost::asio::ip::udp::socket> socket_ptr,
    const SHPunchParam &param,
    const SH_PUNCH_CALLBACK callback)
{
    return Ptr(new AsyncPunchImp3(ios,socket_ptr,param,callback));
}

bool AsyncPunchImp3::do_punch()
{
    bool ret = true;
    do 
    {
        DEBUG_LOG(
            "udptrace", 
            _T("%s Try local address\n"),
            b2w(to_string()).c_str());

        //Send punch request to remote peer's local address.
        ret = send_packet(
            SHUdpCmd_Punch,
            punch_param_.nPeerLocalIp,
            punch_param_.nPeerLocalPort);
        if (!ret)
        {
            break;
        }

        DEBUG_LOG(
            "udptrace", 
            _T("%s Try ports around mapped port\n"),
            b2w(to_string()).c_str());

        //Send punch request to remote peer's mapped port,and around 10 ports.
        uint16_t peer_host_order_mapped_port = ntohs(punch_param_.nPeerMapPort);
        for (int i = 0;i <= kGuessPortCount;++i)
        {
            send_packet(
                SHUdpCmd_Punch,
                punch_param_.nPeerMapIp,
                htons(peer_host_order_mapped_port + i));

            send_packet(
                SHUdpCmd_Punch,
                punch_param_.nPeerMapIp,
                htons(peer_host_order_mapped_port - i));
        }
    } while (0);
    return ret;
}
