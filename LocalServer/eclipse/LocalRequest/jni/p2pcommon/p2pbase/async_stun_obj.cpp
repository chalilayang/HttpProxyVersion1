#include "async_stun_obj.h"
#include "punch_protocol_imp.h"
#include "../base/algorithm.h"
#include "../log/log.h"
#include "punchable.h"

AsyncBindingObj::AsyncBindingObj(
    boost::asio::io_service &ios,
    boost::shared_ptr<boost::asio::ip::udp::socket> socket_ptr,
    const SH_MAPIP_CALLBACK callback,
    uint32_t transaction_id,
    bool change_ip,
    bool change_port,
    uint32_t stun_ip,
    uint16_t stun_port):
	native_socket_(socket_ptr->native()),
	asio_socket_(socket_ptr),
    resolver_(ios),
    callback_(callback),
    binding_timer_(AsyncWaitTimer::create(ios)),
    timeout_counter_(0),
    resolve_time_(0),
    rcv_flag_(false),
    transaction_id_(transaction_id),
    change_ip_(change_ip),
    change_port_(change_port),
    stun_specified_(false),
	is_running_(false)
{
    //DEBUG_LOG("udptrace",_T("%s[%x] AsyncBindingObj created\n"),b2w(to_string()).c_str(),this);
    if (stun_ip != 0 && stun_port != 0)
    {
        stun_endpoint_ = udp::endpoint(boost::asio::ip::address_v4(ntohl(stun_ip)),ntohs(stun_port));
        stun_specified_ = true;
    }
}

AsyncBindingObj::~AsyncBindingObj()
{
    //DEBUG_LOG("udptrace",_T("%s[%x] AsyncBindingObj destroyed\n"),b2w(to_string()).c_str(),this);
}

bool AsyncBindingObj::binding()
{
    bool ret = true;
    do 
    {
        if (!stun_specified_)
        {
            resolve();
        }
        else
        {
            active_binding();
        }

		is_running_ = true;
    } while (0);

    return ret;
}

bool AsyncBindingObj::resolve()
{
    //DEBUG_LOG("download",_T("%s Resolving %s:%s\n"),b2w(to_string()).c_str(),b2w(kStunServerHost).c_str(),b2w(kStunServerUrlPort).c_str());

    //Compose resolve query
    udp::resolver::query query(kStunServerHost, kStunServerUrlPort);

    //Start timing resolve
    spend_time_.restart();

    //Under linux,by default,a call to async_resolve() resulted in 4 queries 5 seconds apart. 
    //The handler was called 20 seconds after the request with the error boost::asio::error::host_not_found.
    resolver_.async_resolve(
        query,
        boost::bind(
        &AsyncBindingObj::handle_resolve,
        shared_from_this(),
        boost::asio::placeholders::error,
        boost::asio::placeholders::iterator));
    return true;
}

bool AsyncBindingObj::handle_resolve(const boost::system::error_code& error_code, udp::resolver::iterator stun_iterator)
{
    bool ret = true;
    do 
    {
		if (!is_running_)
		{
			break;
		}

        if (error_code)
        {
            ERROR_LOG(
                "udptrace",
                _T("%s Resolve error:%d,spend time:%dms\n"),
                b2w(to_string()).c_str(),
                error_code.value(),
                (int32_t)spend_time_.elapsed());
            on_binding_failed();
            ret = false;
            break;
        }

        stun_endpoint_ = *stun_iterator;
        stun_endpoint_.port(kStunServerRealPort); //Set 3478
        resolve_time_ = (int32_t)spend_time_.elapsed();
//         DEBUG_LOG(
//             "udptrace",
//             _T("%s Got stun server %s:%d,spend time:%dms\n"),
//             b2w(to_string()).c_str(),
//             b2w(stun_endpoint_.address().to_string()).c_str(),
//             stun_endpoint_.port(),
//             resolve_time_);

        //Assign raw socket to boost::asio::ip::udp obj
        //asio_socket_->assign(boost::asio::ip::udp::v4(),native_socket_);

        //Do actual binding operation.
        active_binding();
    } while (0);

    if (!ret)
    {
        on_binding_failed();
    }
    return ret;
}

void AsyncBindingObj::active_binding()
{
    spend_time_.restart();
    int32_t timer_count = init_timer();
	if (binding_timer_ != NULL)
	{
		binding_timer_->set_wait_seconds(1); //Not really 1 second,just to active timer,later this time will be reset.
		binding_timer_->set_wait_times(timer_count + 1);
		binding_timer_->async_wait(boost::bind(&AsyncBindingObj::handle_timeout,shared_from_this()));
	}
}

int32_t AsyncBindingObj::init_timer()
{
    int32_t timer_count = 0;
    int32_t timer_step = 100;
    int32_t total_time_length = 0;
    timer_step_vector_.clear();
    while (total_time_length < kBindingTimeout)
    {
        if ((timer_step + total_time_length) > kBindingTimeout)
        {
            timer_step = (kBindingTimeout - total_time_length);
        }

        timer_step_vector_.push_back(timer_step);
        total_time_length += timer_step;
        timer_count++;
        timer_step *= kBindingTimerIncreaseCoefficient;
    }
    return timer_count;
}

bool AsyncBindingObj::handle_timeout()
{
    bool ret = true;
    do 
    {        
		if (!is_running_)
		{
			break;
		}

        timeout_counter_++;
        if (timeout_counter_ > 1)
        {
            DEBUG_LOG("udptrace",_T("%s timeout for %dth time\n"),b2w(to_string()).c_str(),timeout_counter_ - 1);
        }

        if (timeout_counter_ <= (int32_t)timer_step_vector_.size())
        {
            binding_timer_->set_wait_millseconds(timer_step_vector_[timeout_counter_ - 1]);
            ret = do_binding();
            break;
        }

        ret = on_binding_failed();
    } while (0);
    return ret;
}

bool AsyncBindingObj::do_binding()
{
    bool ret = true;
    do 
    {
        //Must send first and receive later,so tcp/ip stack will bind a random port,
        //or boost asio will report err.
        send_packet(kBindRequestMsg);
        async_rcv();
    } while (0);
    return ret;
}

bool AsyncBindingObj::stop()
{
    bool ret = true;
    do 
    {
        if (binding_timer_)
        {
            binding_timer_->cancel();
            binding_timer_.reset();
        }

		resolver_.cancel();
		is_running_ = false;
    } while (0);
    return ret;
}

bool AsyncBindingObj::send_packet(int16_t packet_type)
{
    bool ret = true;
    do 
    {
        char buffer[64] = {0}; //No more then 20 bytes when do binding.
        int32_t length = sizeof(buffer);

        ret = PunchProtocolImp::encode_binding(kBindRequestMsg,buffer,length,transaction_id_,change_ip_,change_port_);
        if (!ret)
        {
            ret  = false;
            break;
        }

        DEBUG_LOG(
            "udptrace",
            _T("%s Send binding packet to %s:%d type:%d\n"),
            b2w(to_string()).c_str(),
            b2w(stun_endpoint_.address().to_string()).c_str(),
            stun_endpoint_.port(),
            packet_type);

        asio_socket_->async_send_to(
            boost::asio::buffer(buffer,length),
            stun_endpoint_,
            boost::bind(
                &AsyncBindingObj::handle_send_packet,
                shared_from_this(),
                boost::asio::placeholders::error,
                boost::asio::placeholders::bytes_transferred));
    } while (0);
    return ret;
}

bool AsyncBindingObj::handle_send_packet(const boost::system::error_code &error,std::size_t bytes_transferred)
{
    bool ret = true;
    do 
    {
    } while (0);
    return ret;
}

void AsyncBindingObj::reset_buffer()
{
    memset(rcv_buffer_,0,sizeof(rcv_buffer_));
}

bool AsyncBindingObj::async_rcv()
{
    bool ret = true;
    do 
    {
        //Control sequence receive operation,don't mess up with the receive buffer.
        if (rcv_flag_)
        {
            break;
        }

        rcv_flag_ = true;
        reset_buffer();
        asio_socket_->async_receive_from(
            boost::asio::buffer(rcv_buffer_,sizeof(rcv_buffer_)),
            rcv_end_point_,
            boost::bind(
            &AsyncBindingObj::handle_rcv_stun_packet,
            shared_from_this(),
            boost::asio::placeholders::error,
            boost::asio::placeholders::bytes_transferred));
    } while (0);
    return ret;
}

bool AsyncBindingObj::handle_rcv_stun_packet(const boost::system::error_code &error,std::size_t bytes_transferred)
{
    bool ret = true;
    do 
    {
		if (!is_running_)
		{
			break;
		}

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

//         DEBUG_LOG(
//             "udptrace",
//             _T("%s Receive %d byte from %s:%u\n"),
//             b2w(to_string()).c_str(),
//             bytes_transferred,
//             b2w(rcv_end_point_.address().to_string()).c_str(),
//             rcv_end_point_.port());

        uint32_t transaction_id;
        uint32_t mapped_ip;
        uint16_t mapped_port = 0;
        uint32_t source_ip;
        uint16_t source_port = 0;
        uint32_t changed_ip;
        uint16_t changed_port = 0;
        ret = PunchProtocolImp::decode_binding_rsp(
            rcv_buffer_,
            bytes_transferred,
            transaction_id,
            mapped_ip,
            mapped_port,
            source_ip,
            source_port,
            changed_ip,
            changed_port);
        if (!ret)
        {
            async_rcv(); //Not stun message,receive more.
            break;
        }

		if (transaction_id != transaction_id_)
		{
			async_rcv(); //Not expected message,receive more.
			break;
		}

        on_binding_success(transaction_id,mapped_ip,mapped_port,source_ip,source_port,changed_ip,changed_port);
    } while (0);

    return ret;
}

bool AsyncBindingObj::on_binding_failed()
{
    bool ret = true;
    do 
    {
        if (binding_timer_)
        {
            binding_timer_->cancel();
            binding_timer_.reset();
        }

        int32_t spend_ms = (int32_t)spend_time_.elapsed();
        int32_t total_ms = spend_ms + resolve_time_;

        DEBUG_LOG(
            "udptrace", 
            _T("%s Binding failed,binding time:%dms,total time:%dms\n"),
            b2w(to_string()).c_str(),
            spend_ms,
            total_ms);

        //Reset stun address in Punchable for using DNS.
        Punchable::set_stun_address(0,0);

        ret = report_fail();
    } while (0);
    return ret;
}

bool AsyncBindingObj::on_binding_success(
    uint32_t transaction_id,
    uint32_t mapped_ip,
    uint16_t mapped_port,
    uint32_t source_ip,
    uint16_t source_port,
    uint32_t changed_ip,
    uint16_t changed_port)
{
    bool ret = true;
    do 
    {
        if (binding_timer_)
        {
            binding_timer_->cancel();
            binding_timer_.reset();
        }

        int32_t spend_ms = (int32_t)spend_time_.elapsed();
        int32_t total_ms = spend_ms + resolve_time_;
        
        DEBUG_LOG(
            "udptrace", 
            _T("%s Binding success,mapped address %s:%u,binding time:%dms,total time:%dms\n"),
			b2w(to_string()).c_str(),
            b2w(uint2ip(mapped_ip)).c_str(),
            ntohs(mapped_port),
            spend_ms,
            total_ms);

        //For reuse stun address for avoiding repeat domain name resolution.
        Punchable::set_stun_address(source_ip,source_port);

		ret = report_success(transaction_id,mapped_ip,mapped_port,source_ip,source_port,changed_ip,changed_port);
    } while (0);
    return ret;
}

std::string AsyncBindingObj::to_string()
{
    do 
    {
        if (!binding_info_.empty())
        {
            break;
        }

        char info[1024] = {0};
        snprintf(
            info,
            sizeof(info),
            "[Binding][%d]",
            native_socket_);
        binding_info_ = info;
    } while (0);

    return binding_info_;
}

void AsyncBindingObj::print_errno(const std::string &function_name)
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

bool AsyncBindingObj::report_success(
    uint32_t transaction_id,
    uint32_t mapped_ip,
    uint16_t mapped_port,
    uint32_t source_ip,
    uint16_t source_port,
    uint32_t changed_ip,
    uint16_t changed_port)
{
    bool ret = true;
    do 
    {
        if (callback_ == NULL)
        {
            break;
        }

        callback_(native_socket_,mapped_ip,mapped_port);
    } while (0);
    return ret;
}

bool AsyncBindingObj::report_fail()
{
    bool ret = true;
    do 
    {
        if (callback_ == NULL)
        {
            break;
        }

        callback_(native_socket_,0,0);
    } while (0);
    return ret;
}
