#ifdef ASYNC_NAT_DETECT
#include "async_nat_detect.h"
#include "async_stun_obj.h"
#include "../base/algorithm.h"
#include "../log/log.h"

#ifdef __IOS__
#include <ifaddrs.h>
#endif

////////////////////////////////////StunBindingTransaction//////////////////////////////////////
StunBindingTransaction::StunBindingTransaction(
    boost::asio::io_service &ios,
    boost::shared_ptr<boost::asio::ip::udp::socket> socket_ptr,
    uint32_t transaction_id,
    bool change_ip,
    bool change_port,
    uint32_t stun_ip,
    uint16_t stun_port,
    const StunEventCallback &callback,
    uint32_t detector_id):
    AsyncBindingObj(
        ios,
        socket_ptr,
        NULL,
        transaction_id,
        change_ip,
        change_port,
        stun_ip,
        stun_port),
    callback_(callback),
    detector_id_(detector_id)
{

}

StunBindingTransaction::~StunBindingTransaction()
{

}

StunBindingTransaction::Ptr StunBindingTransaction::Create(
    boost::asio::io_service &ios,
    boost::shared_ptr<boost::asio::ip::udp::socket> socket_ptr,
    uint32_t transaction_id,
    bool change_ip,
    bool change_port,
    uint32_t stun_ip,
    uint16_t stun_port,
    const StunEventCallback &callback,
    uint32_t detector_id)
{
    return Ptr(new StunBindingTransaction(ios,socket_ptr,transaction_id,change_ip,change_port,stun_ip,stun_port,callback,detector_id));
}

bool StunBindingTransaction::report_success(
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

        StunEvent ev;
        ev.transaction_id = transaction_id;
        ev.mapped_ip = mapped_ip;
        ev.mapped_port = mapped_port;
        ev.source_ip = source_ip;
        ev.source_port = source_port;
        ev.changed_ip = changed_ip;
        ev.changed_port = changed_port;

        ret = callback_(detector_id_,ev);
    } while (0);
    return ret;
}

bool StunBindingTransaction::report_fail()
{
    bool ret = true;
    do 
    {
        if (callback_ == NULL)
        {
            break;
        }

        StunEvent ev;
		ev.transaction_id = transaction_id_;
        ret = callback_(detector_id_,ev);
    } while (0);
    return ret;
}

////////////////////////////////////AsyncNatDetector//////////////////////////////////////
uint32_t AsyncNatDetector::global_detctor_id_ = 1;
uint32_t AsyncNatDetector::transaction_id_ = 1;
boost::unordered_map<uint32_t,AsyncNatDetector::SharedPtr> AsyncNatDetector::detector_table_;
AsyncNatDetector::AsyncNatDetector(const SH_NAT_DETECT_CALLBACK &callback):
    ios_(server_ios()), //Must create after ios pool created.
    callback_(callback),
    current_state_(kNatDetectState_Idle),
    detector_id_(0)
{
	DEBUG_LOG("protocal",_T("[AC][%x] AsyncNatDetector created\n"),this);
}

AsyncNatDetector::~AsyncNatDetector()
{
    if (socket_ != NULL)
    {
        if (socket_->is_open())
        {
            socket_->close();
            socket_.reset();
        }
    }

	DEBUG_LOG("protocal",_T("[AC][%x] AsyncNatDetector destroyed\n"),this);
}

AsyncNatDetector::SharedPtr AsyncNatDetector::Create(const SH_NAT_DETECT_CALLBACK &callback)
{
    return SharedPtr(new AsyncNatDetector(callback));
}

bool AsyncNatDetector::detect_nat_type(const SH_NAT_DETECT_CALLBACK &callback)
{
    bool ret = true;
    uint32_t detector_id = 0;
    do 
    {
        SharedPtr detector = Create(callback);
        if (detector == NULL)
        {
            ret = false;
            break;
        }

        detector_id = (global_detctor_id_++);
        if (global_detctor_id_ == 0)
        {
            global_detctor_id_++;
        }
        detector_table_[detector_id] = detector;

        ret = detector->init(detector_id);
        if (!ret)
        {
            break;
        }

        ret = detector->activate();
        if (!ret)
        {
            break;
        }
    } while (0);

    if (!ret && detector_table_.find(detector_id) != detector_table_.end())
    {
        detector_table_.erase(detector_id);
        if (callback != NULL)
        {
            callback(NatType_ERROR);
        }
    }

    return ret;
}

bool AsyncNatDetector::init(uint32_t detector_id)
{
    bool ret = true;
    do 
    {
        boost::system::error_code error;
        socket_.reset(new boost::asio::ip::udp::socket(ios_));
        socket_->open(udp::v4(), error);
        if (error)
        {
            socket_->close(error);
            ret = false;
            break;
        }
    
        //Register event transformer.
        REGISTER_NAT_DETECT_EVENT_TRANSFORMER(Idle);
        REGISTER_NAT_DETECT_EVENT_TRANSFORMER(Block_NAT_Checking);
        REGISTER_NAT_DETECT_EVENT_TRANSFORMER(Open_Internet_Checking);
        REGISTER_NAT_DETECT_EVENT_TRANSFORMER(Full_Cone_NAT_Checking);
        REGISTER_NAT_DETECT_EVENT_TRANSFORMER(Symmetric_NAT_Checking);
        REGISTER_NAT_DETECT_EVENT_TRANSFORMER(Restrict_NAT_Checking);

        //Register state transition.
        REGISTER_NAT_DETECT_STATE_TRANSITION(Idle,Activate,Block_NAT_Checking,ERROR);
        REGISTER_NAT_DETECT_STATE_TRANSITION(Block_NAT_Checking,Timeout,Done,BLOCKED);
        REGISTER_NAT_DETECT_STATE_TRANSITION(Block_NAT_Checking,Ip_Not_Changed,Open_Internet_Checking,ERROR);
        REGISTER_NAT_DETECT_STATE_TRANSITION(Block_NAT_Checking,Ip_Changed,Full_Cone_NAT_Checking,ERROR);
        REGISTER_NAT_DETECT_STATE_TRANSITION(Open_Internet_Checking,Timeout,Done,FIREWALL);
        REGISTER_NAT_DETECT_STATE_TRANSITION(Open_Internet_Checking,Received_Response,Done,OPEN_INTERNET);
        REGISTER_NAT_DETECT_STATE_TRANSITION(Full_Cone_NAT_Checking,Received_Response,Done,FULL_CONE);
        REGISTER_NAT_DETECT_STATE_TRANSITION(Full_Cone_NAT_Checking,Timeout,Symmetric_NAT_Checking,ERROR);
        REGISTER_NAT_DETECT_STATE_TRANSITION(Symmetric_NAT_Checking,Timeout,Done,ERROR);
        REGISTER_NAT_DETECT_STATE_TRANSITION(Symmetric_NAT_Checking,Mapped_Address_Changed,Done,SYMMETRIC_NAT);
        REGISTER_NAT_DETECT_STATE_TRANSITION(Symmetric_NAT_Checking,Mapped_Address_Not_Changed,Restrict_NAT_Checking,ERROR);
        REGISTER_NAT_DETECT_STATE_TRANSITION(Restrict_NAT_Checking,Timeout,Done,PORT_RESTRICTED);
        REGISTER_NAT_DETECT_STATE_TRANSITION(Restrict_NAT_Checking,Received_Response,Done,RESTRICTED);

        //
        detector_id_ = detector_id;
    } while (0);
    return ret;
}

bool AsyncNatDetector::activate()
{
    bool ret = true;
    do 
    {
        current_state_ = kNatDetectState_Idle;
        StunEvent ev;
        ret = transit(ev);
    }while (0);
    return ret;
}

bool AsyncNatDetector::transit(const StunEvent &input_event)
{
    bool ret = true;
    do 
    {
        if (socket_ == NULL)
        {
            ret = false;
            break;
        }

		if (transaction_id_ - 1 != input_event.transaction_id)
		{
			DEBUG_LOG("protocal",_T("[AC] Received transaction id %u not match last transaction id %u,ignore\n"),
				input_event.transaction_id,
				transaction_id_ - 1);
			break; //Ignoring events that transaction id not match.
		}

        if (event_transformer_[current_state_] == NULL)
        {
            ret = false;
            break;
        }

        NatDetectEvent output_ev;
        ret = event_transformer_[current_state_](input_event,reference_stun_event_,output_ev);
        if (!ret)
        {
            break;
        }

        if (state_machine_[current_state_][output_ev].action == NULL)
        {
            ret = false;
            break;
        }

        ret = state_machine_[current_state_][output_ev].action(reference_stun_event_,detector_id_,socket_);
        if (!ret)
        {
            break;
        }

        NatDetectState last_state = current_state_;
        current_state_ = state_machine_[current_state_][output_ev].next_state;
        if (current_state_ != kNatDetectState_Done)
        {
            break;
        }

        detector_table_.erase(detector_id_);
        on_detect_result(state_machine_[last_state][output_ev].nat_type);
    } while (0);
    return ret;
}

bool AsyncNatDetector::on_detect_result(SHNatType nat_type)
{
    bool ret = true;
    do 
    {
        if (callback_ == NULL)
        {
            break;
        }

        callback_(nat_type);
    } while (0);
    return ret;
}

bool AsyncNatDetector::handle_stun_event(uint32_t detector_id,const StunEvent &input_event)
{
    bool ret = true;
    boost::unordered_map<uint32_t,SharedPtr>::iterator iter = detector_table_.end();
    do 
    {
		DEBUG_LOG("protocal",_T("[AC] Got stun event tranaction id:%u,mapped address:%s:%d,source address:%s:%d,changed address:%s:%d\n"),
			input_event.transaction_id,
			b2w(uint2ip(input_event.mapped_ip)).c_str(),
			ntohs(input_event.mapped_port),
			b2w(uint2ip(input_event.source_ip)).c_str(),
			ntohs(input_event.source_port),
			b2w(uint2ip(input_event.changed_ip)).c_str(),
			ntohs(input_event.changed_port));

        iter = detector_table_.find(detector_id);
        if (iter == detector_table_.end())
        {
            ret = false;
            break;
        }

        SharedPtr ptr = iter->second;
        if (ptr == NULL)
        {
            ret = false;
            break;
        }

        ret = ptr->transit(input_event);
        if (!ret)
        {
            detector_table_.erase(iter);
            ptr->on_detect_result(NatType_ERROR);
        }
    } while (0);

    return ret;
}

bool AsyncNatDetector::start_stun_binding_transaction(uint32_t detector_id,SocketPtr sock,bool change_ip,bool change_port,uint32_t stun_ip,uint16_t stun_port)
{
    bool ret = true;
    do 
    {
        StunBindingTransaction::Ptr binding_transaction = StunBindingTransaction::Create(
            sock->get_io_service(),
            sock,
            transaction_id_++,
            change_ip,
            change_port,
            stun_ip,
            stun_port,
            boost::bind(&AsyncNatDetector::handle_stun_event,_1,_2),
            detector_id);

		DEBUG_LOG("protocal",_T("[AC] Start binding transaction id %u,change ip:%s,changeport:%s,stun:%s:%u\n"),
			transaction_id_ - 1,
			change_ip?_T("true"):_T("false"),
			change_port?_T("true"):_T("false"),
			(stun_ip == 0)?_T("0"):b2w(uint2ip(stun_ip)).c_str(),
			ntohs(stun_port));

		if (transaction_id_ == 0)
		{
			transaction_id_++;
		}

        ret = binding_transaction->binding();
    } while (0);
    return ret;
}

bool AsyncNatDetector::is_match_local_ip(uint32_t mapped_ip)
{
    bool ret = false;
    do 
    {
        if (mapped_ip == 0)
        {
            break;
        }

#ifdef WIN32
        char host_name[256] = {0};
        if(gethostname(host_name,sizeof(host_name)) != 0)
        {
            break;
        }

        hostent* host_info = gethostbyname(host_name);
        if (host_info == NULL)
        {
            break;
        }

        for(size_t i = 0; i < host_info->h_length / sizeof(int); ++i)
        {
            if ((*(uint32_t*)host_info->h_addr_list[i]) == mapped_ip)
            {
                ret = true;
                break;
            }
        }
#else
        struct ifaddrs *interfaces = NULL;
        do 
        {
            // Get the current interfaces - returns 0 on success.
            int32_t success = getifaddrs(&interfaces);
            if (success != 0)
            {
                break;
            }

            // Loop through linked list of interfaces.
            struct ifaddrs *iter_addr = interfaces;
            while (iter_addr != NULL)
            {
                if (((struct sockaddr_in *)iter_addr->ifa_addr) != NULL && 
                    (((struct sockaddr_in *)iter_addr->ifa_addr)->sin_addr.s_addr) == mapped_ip)
                {
                    ret = true;
                    break;
                }
                iter_addr = iter_addr->ifa_next;
            }
        } while (0);

        // Free memory
        if (interfaces != NULL)
        {
            freeifaddrs(interfaces);
        }
#endif
    } while (0);
    return ret;
}

bool AsyncNatDetector::event_transformer_Idle(const StunEvent &input_ev,StunEvent &reference_stun_ev,NatDetectEvent &output_ev)
{
    bool ret = true;
    do 
    {
        output_ev = kNatDetectEvent_Activate;
    } while (0);
    return ret;
}

bool AsyncNatDetector::event_transformer_Block_NAT_Checking(const StunEvent &input_ev,StunEvent &reference_stun_ev,NatDetectEvent &output_ev)
{
    bool ret = true;
    do 
    {
        if (input_ev.changed_ip == 0 && 
            input_ev.changed_port == 0 &&
            input_ev.source_ip == 0 &&
            input_ev.source_port == 0 &&
            input_ev.mapped_ip == 0 &&
            input_ev.mapped_port == 0)
        {
            output_ev = kNatDetectEvent_Timeout;
            break;
        }

        //Store reference stun event here.
        reference_stun_ev = input_ev;

        if (is_match_local_ip(input_ev.mapped_ip))
        {
            output_ev = kNatDetectEvent_Ip_Not_Changed;
            break;
        }

        output_ev = kNatDetectEvent_Ip_Changed;
    } while (0);
    return ret;
}

bool AsyncNatDetector::event_transformer_Open_Internet_Checking(const StunEvent &input_ev,StunEvent &reference_stun_ev,NatDetectEvent &output_ev)
{
    bool ret = true;
    do 
    {
        if (input_ev.changed_ip == 0 && 
            input_ev.changed_port == 0 &&
            input_ev.source_ip == 0 &&
            input_ev.source_port == 0 &&
            input_ev.mapped_ip == 0 &&
            input_ev.mapped_port == 0)
        {
            output_ev = kNatDetectEvent_Timeout;
            break;
        }

        output_ev = kNatDetectEvent_Received_Response;
    } while (0);
    return ret;
}

bool AsyncNatDetector::event_transformer_Full_Cone_NAT_Checking(const StunEvent &input_ev,StunEvent &reference_stun_ev,NatDetectEvent &output_ev)
{
    bool ret = true;
    do 
    {
        if (input_ev.changed_ip == 0 && 
            input_ev.changed_port == 0 &&
            input_ev.source_ip == 0 &&
            input_ev.source_port == 0 &&
            input_ev.mapped_ip == 0 &&
            input_ev.mapped_port == 0)
        {
            output_ev = kNatDetectEvent_Timeout;
            break;
        }

        output_ev = kNatDetectEvent_Received_Response;
    } while (0);
    return ret;
}

bool AsyncNatDetector::event_transformer_Symmetric_NAT_Checking(const StunEvent &input_ev,StunEvent &reference_stun_ev,NatDetectEvent &output_ev)
{
    bool ret = true;
    do 
    {
        if (input_ev.changed_ip == 0 && 
            input_ev.changed_port == 0 &&
            input_ev.source_ip == 0 &&
            input_ev.source_port == 0 &&
            input_ev.mapped_ip == 0 &&
            input_ev.mapped_port == 0)
        {
            output_ev = kNatDetectEvent_Timeout;
            break;
        }

        if (reference_stun_ev.mapped_ip != input_ev.mapped_ip || reference_stun_ev.mapped_port != input_ev.mapped_port)
        {
            output_ev = kNatDetectEvent_Mapped_Address_Changed;
            break;
        }

        output_ev = kNatDetectEvent_Mapped_Address_Not_Changed;
    } while (0);
    return ret;
}

bool AsyncNatDetector::event_transformer_Restrict_NAT_Checking(const StunEvent &input_ev,StunEvent &reference_stun_ev,NatDetectEvent &output_ev)
{
    bool ret = true;
    do 
    {
        if (input_ev.changed_ip == 0 && 
            input_ev.changed_port == 0 &&
            input_ev.source_ip == 0 &&
            input_ev.source_port == 0 &&
            input_ev.mapped_ip == 0 &&
            input_ev.mapped_port == 0)
        {
            output_ev = kNatDetectEvent_Timeout;
            break;
        }

        output_ev = kNatDetectEvent_Received_Response;
    } while (0);
    return ret;
}

bool AsyncNatDetector::transit_action_Idle_Activate_Block_NAT_Checking(const StunEvent &reference_stun_ev,uint32_t detector_id,SocketPtr sock)
{
    bool ret = true;
    do 
    {
		DEBUG_LOG("protocal",_T("[AC] transit_action_Idle_Activate_Block_NAT_Checking\n"));
        ret = start_stun_binding_transaction(detector_id,sock,false,false);
    } while (0);
    return ret;
}

bool AsyncNatDetector::transit_action_Block_NAT_Checking_Timeout_Done(const StunEvent &reference_stun_ev,uint32_t detector_id,SocketPtr sock)
{
    bool ret = true;
    do 
    {
        DEBUG_LOG("protocal",_T("[AC] transit_action_Block_NAT_Checking_Timeout_Done\n"));
    } while (0);
    return ret;
}

bool AsyncNatDetector::transit_action_Block_NAT_Checking_Ip_Not_Changed_Open_Internet_Checking(const StunEvent &reference_stun_ev,uint32_t detector_id,SocketPtr sock)
{
    bool ret = true;
    do 
    {
		DEBUG_LOG("protocal",_T("[AC] transit_action_Block_NAT_Checking_Ip_Not_Changed_Open_Internet_Checking\n"));
        ret = start_stun_binding_transaction(detector_id,sock,true,true,reference_stun_ev.source_ip,reference_stun_ev.source_port);
    } while (0);
    return ret;
}

bool AsyncNatDetector::transit_action_Block_NAT_Checking_Ip_Changed_Full_Cone_NAT_Checking(const StunEvent &reference_stun_ev,uint32_t detector_id,SocketPtr sock)
{
    bool ret = true;
    do 
    {
		DEBUG_LOG("protocal",_T("[AC] transit_action_Block_NAT_Checking_Ip_Changed_Full_Cone_NAT_Checking\n"));
        ret = start_stun_binding_transaction(detector_id,sock,true,true,reference_stun_ev.source_ip,reference_stun_ev.source_port);
    } while (0);
    return ret;
}

bool AsyncNatDetector::transit_action_Open_Internet_Checking_Timeout_Done(const StunEvent &reference_stun_ev,uint32_t detector_id,SocketPtr sock)
{
    bool ret = true;
    do 
    {
        DEBUG_LOG("protocal",_T("[AC] transit_action_Open_Internet_Checking_Timeout_Done\n"));
    } while (0);
    return ret;
}

bool AsyncNatDetector::transit_action_Open_Internet_Checking_Received_Response_Done(const StunEvent &reference_stun_ev,uint32_t detector_id,SocketPtr sock)
{
    bool ret = true;
    do 
    {
        DEBUG_LOG("protocal",_T("[AC] transit_action_Open_Internet_Checking_Received_Response_Done\n"));
    } while (0);
    return ret;
}

bool AsyncNatDetector::transit_action_Full_Cone_NAT_Checking_Received_Response_Done(const StunEvent &reference_stun_ev,uint32_t detector_id,SocketPtr sock)
{
    bool ret = true;
    do 
    {
        DEBUG_LOG("protocal",_T("[AC] transit_action_Full_Cone_NAT_Checking_Received_Response_Done\n"));
    } while (0);
    return ret;
}

bool AsyncNatDetector::transit_action_Full_Cone_NAT_Checking_Timeout_Symmetric_NAT_Checking(const StunEvent &reference_stun_ev,uint32_t detector_id,SocketPtr sock)
{
    bool ret = true;
    do 
    {
		DEBUG_LOG("protocal",_T("[AC] transit_action_Full_Cone_NAT_Checking_Timeout_Symmetric_NAT_Checking\n"));
        ret = start_stun_binding_transaction(detector_id,sock,false,false,reference_stun_ev.changed_ip,reference_stun_ev.changed_port);
    } while (0);
    return ret;
}

bool AsyncNatDetector::transit_action_Symmetric_NAT_Checking_Timeout_Done(const StunEvent &reference_stun_ev,uint32_t detector_id,SocketPtr sock)
{
    bool ret = true;
    do 
    {
        DEBUG_LOG("protocal",_T("[AC] transit_action_Symmetric_NAT_Checking_Timeout_Done\n"));
    } while (0);
    return ret;
}

bool AsyncNatDetector::transit_action_Symmetric_NAT_Checking_Mapped_Address_Changed_Done(const StunEvent &reference_stun_ev,uint32_t detector_id,SocketPtr sock)
{
    bool ret = true;
    do 
    {
        DEBUG_LOG("protocal",_T("[AC] transit_action_Symmetric_NAT_Checking_Mapped_Address_Changed_Done\n"));
    } while (0);
    return ret;
}

bool AsyncNatDetector::transit_action_Symmetric_NAT_Checking_Mapped_Address_Not_Changed_Restrict_NAT_Checking(const StunEvent &reference_stun_ev,uint32_t detector_id,SocketPtr sock)
{
    bool ret = true;
    do 
    {
		DEBUG_LOG("protocal",_T("[AC] transit_action_Symmetric_NAT_Checking_Mapped_Address_Not_Changed_Restrict_NAT_Checking\n"));
        ret = start_stun_binding_transaction(detector_id,sock,false,true,reference_stun_ev.source_ip,reference_stun_ev.source_port);
    } while (0);
    return ret;
}

bool AsyncNatDetector::transit_action_Restrict_NAT_Checking_Timeout_Done(const StunEvent &reference_stun_ev,uint32_t detector_id,SocketPtr sock)
{
    bool ret = true;
    do 
    {
        DEBUG_LOG("protocal",_T("[AC] transit_action_Restrict_NAT_Checking_Timeout_Done\n"));
    } while (0);
    return ret;
}

bool AsyncNatDetector::transit_action_Restrict_NAT_Checking_Received_Response_Done(const StunEvent &reference_stun_ev,uint32_t detector_id,SocketPtr sock)
{
    bool ret = true;
    do 
    {
        DEBUG_LOG("protocal",_T("[AC] transit_action_Restrict_NAT_Checking_Received_Response_Done\n"));
    } while (0);
    return ret;
}
#endif // #ifdef ASYNC_NAT_DETECT
