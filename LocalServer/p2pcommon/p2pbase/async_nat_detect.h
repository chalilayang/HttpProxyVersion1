#ifdef ASYNC_NAT_DETECT
#ifndef __ASYNC_NAT_DETECT_H__
#define __ASYNC_NAT_DETECT_H__

#include "../base/common.h"
#include "../p2pbase/udp_def.h"
#include "../base/timer.h"
#include "async_stun_obj.h"

////////////////////////////////////Type Definition//////////////////////////////////////
//State.
typedef enum NatDetectState
{
    kNatDetectState_Idle = 0,
    kNatDetectState_Block_NAT_Checking,
    kNatDetectState_Open_Internet_Checking,
    kNatDetectState_Full_Cone_NAT_Checking,
    kNatDetectState_Symmetric_NAT_Checking,
    kNatDetectState_Restrict_NAT_Checking,
    kNatDetectState_Done,
    kNatDetectState_Count
}NatDetectState;

//Event.
typedef enum NatDetectEvent
{
    kNatDetectEvent_Activate = 0,
    kNatDetectEvent_Timeout,
    kNatDetectEvent_Received_Response,
    kNatDetectEvent_Ip_Not_Changed,
    kNatDetectEvent_Ip_Changed,
    kNatDetectEvent_Mapped_Address_Not_Changed,
    kNatDetectEvent_Mapped_Address_Changed,
    kNatDetectEvent_Count
}NatDetectEvent;

//Stun Event.
typedef struct StunEvent
{
    uint32_t transaction_id;
    uint32_t mapped_ip;
    uint16_t mapped_port;
    uint32_t source_ip;
    uint16_t source_port;
    uint32_t changed_ip;
    uint16_t changed_port;

    StunEvent()
    {
        transaction_id = 0;
        mapped_ip = 0;
        mapped_port = 0;
        source_ip = 0;
        source_port = 0;
        changed_ip = 0;
        changed_port = 0;
    }
}StunEvent;

//Event transformer.
typedef boost::function<bool(const StunEvent &input_ev,StunEvent &reference_stun_ev,NatDetectEvent &output_ev)> NatDetectEventTransformer;

//Do something under certain state and transit to next state.
typedef boost::function<bool(const StunEvent &reference_stun_ev,uint32_t detector_id,boost::shared_ptr<boost::asio::ip::udp::socket> sock)> NatDetectAction;

//Stun event callback.
typedef boost::function<bool(uint32_t detector_id,const StunEvent &ev)> StunEventCallback;

//State transition Agent.
typedef struct NatDetectStateTransitionAgent
{
    NatDetectAction action;
    NatDetectState next_state;
    SHNatType nat_type;

    NatDetectStateTransitionAgent()
    {
        next_state = kNatDetectState_Idle;
        nat_type = NatType_FULL_CONE;
    }
}NatDetectStateTransitionAgent;

//Macro to simplify add event transformer.
#define REGISTER_NAT_DETECT_EVENT_TRANSFORMER(st)\
do\
{\
    event_transformer_[kNatDetectState_##st] = boost::bind(&AsyncNatDetector::event_transformer_##st,_1,_2,_3);\
}while(0)

//Macro to simplify add state transition agent.
//Under state "cur_str",the "ev" event activates certain action and transits to next state "next_st",if next state is Done.
#define REGISTER_NAT_DETECT_STATE_TRANSITION(cur_st,ev,next_st,nat)\
do\
{\
    state_machine_[kNatDetectState_##cur_st][kNatDetectEvent_##ev].action = boost::bind(&AsyncNatDetector::transit_action_##cur_st##_##ev##_##next_st,_1,_2,_3);\
    state_machine_[kNatDetectState_##cur_st][kNatDetectEvent_##ev].next_state = kNatDetectState_##next_st;\
    state_machine_[kNatDetectState_##cur_st][kNatDetectEvent_##ev].nat_type = NatType_##nat;\
}while(0)

///////////////////////////////////StunBindingTransaction///////////////////////////////////////
class StunBindingTransaction : public AsyncBindingObj
{
public:
    typedef boost::shared_ptr<StunBindingTransaction> Ptr;

    static Ptr Create(
        boost::asio::io_service &ios,
        boost::shared_ptr<boost::asio::ip::udp::socket> socket_ptr,
        uint32_t transaction_id,
        bool change_ip,
        bool change_port,
        uint32_t stun_ip,
        uint16_t stun_port,
        const StunEventCallback &callback,
        uint32_t detector_id);

    StunBindingTransaction(
        boost::asio::io_service &ios,
        boost::shared_ptr<boost::asio::ip::udp::socket> socket_ptr,
        uint32_t transaction_id,
        bool change_ip,
        bool change_port,
        uint32_t stun_ip,
        uint16_t stun_port,
        const StunEventCallback &callback,
        uint32_t detector_id);

    ~StunBindingTransaction();

protected:
    virtual bool report_success(
        uint32_t transaction_id,
        uint32_t mapped_ip,
        uint16_t mapped_port,
        uint32_t source_ip,
        uint16_t source_port,
        uint32_t changed_ip,
        uint16_t changed_port);

    virtual bool report_fail();

private:
    StunEventCallback callback_;
    uint32_t detector_id_;
};

/////////////////////////////////////AsyncNatDetector/////////////////////////////////////
class AsyncNatDetector : public boost::enable_shared_from_this<AsyncNatDetector>
{
public:
    typedef boost::shared_ptr<AsyncNatDetector> SharedPtr;
    typedef boost::shared_ptr<boost::asio::ip::udp::socket> SocketPtr;
    static SharedPtr Create(const SH_NAT_DETECT_CALLBACK &callback);
    AsyncNatDetector(const SH_NAT_DETECT_CALLBACK &callback);
    ~AsyncNatDetector();

public:
    //Interface.
    static bool detect_nat_type(const SH_NAT_DETECT_CALLBACK &callback);

private:
    bool init(uint32_t detector_id);
    bool activate();
    bool transit(const StunEvent &input_event);
    bool on_detect_result(SHNatType nat_type);

    static bool handle_stun_event(uint32_t detector_id,const StunEvent &input_event);
    static bool start_stun_binding_transaction(uint32_t detector_id,SocketPtr sock,bool change_ip,bool change_port,uint32_t stun_ip = 0,uint16_t stun_port = 0);
    static bool is_match_local_ip(uint32_t mapped_ip);

    static bool event_transformer_Idle(const StunEvent &input_ev,StunEvent &reference_stun_ev,NatDetectEvent &output_ev);
    static bool event_transformer_Block_NAT_Checking(const StunEvent &input_ev,StunEvent &reference_stun_ev,NatDetectEvent &output_ev);
    static bool event_transformer_Open_Internet_Checking(const StunEvent &input_ev,StunEvent &reference_stun_ev,NatDetectEvent &output_ev);
    static bool event_transformer_Full_Cone_NAT_Checking(const StunEvent &input_ev,StunEvent &reference_stun_ev,NatDetectEvent &output_ev);
    static bool event_transformer_Symmetric_NAT_Checking(const StunEvent &input_ev,StunEvent &reference_stun_ev,NatDetectEvent &output_ev);
    static bool event_transformer_Restrict_NAT_Checking(const StunEvent &input_ev,StunEvent &reference_stun_ev,NatDetectEvent &output_ev);

    static bool transit_action_Idle_Activate_Block_NAT_Checking(const StunEvent &reference_stun_ev,uint32_t detector_id,SocketPtr sock);
    static bool transit_action_Block_NAT_Checking_Timeout_Done(const StunEvent &reference_stun_ev,uint32_t detector_id,SocketPtr sock);
    static bool transit_action_Block_NAT_Checking_Ip_Not_Changed_Open_Internet_Checking(const StunEvent &reference_stun_ev,uint32_t detector_id,SocketPtr sock);
    static bool transit_action_Block_NAT_Checking_Ip_Changed_Full_Cone_NAT_Checking(const StunEvent &reference_stun_ev,uint32_t detector_id,SocketPtr sock);
    static bool transit_action_Open_Internet_Checking_Timeout_Done(const StunEvent &reference_stun_ev,uint32_t detector_id,SocketPtr sock);
    static bool transit_action_Open_Internet_Checking_Received_Response_Done(const StunEvent &reference_stun_ev,uint32_t detector_id,SocketPtr sock);
    static bool transit_action_Full_Cone_NAT_Checking_Received_Response_Done(const StunEvent &reference_stun_ev,uint32_t detector_id,SocketPtr sock);
    static bool transit_action_Full_Cone_NAT_Checking_Timeout_Symmetric_NAT_Checking(const StunEvent &reference_stun_ev,uint32_t detector_id,SocketPtr sock);
    static bool transit_action_Symmetric_NAT_Checking_Timeout_Done(const StunEvent &reference_stun_ev,uint32_t detector_id,SocketPtr sock);
    static bool transit_action_Symmetric_NAT_Checking_Mapped_Address_Changed_Done(const StunEvent &reference_stun_ev,uint32_t detector_id,SocketPtr sock);
    static bool transit_action_Symmetric_NAT_Checking_Mapped_Address_Not_Changed_Restrict_NAT_Checking(const StunEvent &reference_stun_ev,uint32_t detector_id,SocketPtr sock);
    static bool transit_action_Restrict_NAT_Checking_Timeout_Done(const StunEvent &reference_stun_ev,uint32_t detector_id,SocketPtr sock);
    static bool transit_action_Restrict_NAT_Checking_Received_Response_Done(const StunEvent &reference_stun_ev,uint32_t detector_id,SocketPtr sock);

private:
    SocketPtr socket_;
    boost::asio::io_service& ios_;
    SH_NAT_DETECT_CALLBACK callback_;
    NatDetectEventTransformer event_transformer_[kNatDetectState_Count];
    NatDetectStateTransitionAgent state_machine_[kNatDetectState_Count][kNatDetectEvent_Count]; 
    NatDetectState current_state_;
    StunEvent reference_stun_event_; //For storing last stun state.
    uint32_t detector_id_; //Accompany with detector_table_,decoupling with class StunBindingTransaction,there won't be any real reference of AsyncNatDetector object but this id.
    static uint32_t global_detctor_id_;
    static uint32_t transaction_id_;
    static boost::unordered_map<uint32_t,SharedPtr> detector_table_;
};

#endif // #ifndef __ASYNC_NAT_DETECT_H__
#endif // #ifdef ASYNC_NAT_DETECT
