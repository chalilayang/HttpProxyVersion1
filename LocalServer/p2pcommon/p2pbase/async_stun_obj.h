#ifndef __ASYNC_STUN_OBJ_H__
#define __ASYNC_STUN_OBJ_H__

#include "udp_def.h"
#include "async_punch_obj.h"
#include "../sh_kernel.h"
#include "../download/peer_connection.h"
#include "../base/ioservice_pool.h"
#include "../base/common.h"
#include "../base/timer.h"

////////////////////////////////Constant Definition//////////////////////////////////////////
const int32_t kBindingTimeout = 1600;
const int32_t kBindingTimerIncreaseCoefficient = 2;
const std::string kStunServerHost = "stun.p2p.hd.sohu.com";
const std::string kStunServerUrlPort = "80";
const int16_t kStunServerRealPort = 3478;

//////////////////////////////////////AsyncBindingObj////////////////////////////////////
class AsyncBindingObj : public boost::enable_shared_from_this<AsyncBindingObj>
{
public:
    AsyncBindingObj(
        boost::asio::io_service &ios,
		boost::shared_ptr<boost::asio::ip::udp::socket> socket_ptr,
        const SH_MAPIP_CALLBACK callback,
        uint32_t transaction_id = 0,
        bool change_ip = false,
        bool change_port = false,
        uint32_t stun_ip = 0,
        uint16_t stun_port = 0);
    virtual ~AsyncBindingObj();

public:
    bool binding();
    bool stop();

protected:
    virtual bool do_binding();
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
    bool resolve();
    bool handle_resolve(const boost::system::error_code& error_code, udp::resolver::iterator endpoint_iterator);
    void active_binding();
    int32_t init_timer();
    bool handle_timeout();
    bool send_packet(int16_t packet_type);
    bool handle_send_packet(const boost::system::error_code &error,std::size_t bytes_transferred);
    void reset_buffer();
    bool async_rcv();
    bool handle_rcv_stun_packet(const boost::system::error_code &error,std::size_t bytes_transferred);
    bool on_binding_failed();
    bool on_binding_success(
        uint32_t transaction_id,
        uint32_t mapped_ip,
        uint16_t mapped_port,
        uint32_t source_ip,
        uint16_t source_port,
        uint32_t changed_ip,
        uint16_t changed_port);
    std::string to_string();
    void print_errno(const std::string &function_name);

protected:
    SOCKET native_socket_;
	boost::shared_ptr<boost::asio::ip::udp::socket> asio_socket_;
    udp::resolver resolver_;
    SH_MAPIP_CALLBACK callback_;
    boost::asio::ip::udp::endpoint rcv_end_point_;
    char rcv_buffer_[kUDPMTUSize];
    MillisecTimer spend_time_;
    AsyncWaitTimer::Ptr binding_timer_;
    int32_t timeout_counter_;
    std::string binding_info_;
    std::vector<int32_t> timer_step_vector_;
    udp::endpoint stun_endpoint_;
    int32_t resolve_time_;
    bool rcv_flag_;
    uint32_t transaction_id_;
    bool change_ip_;
    bool change_port_;
    bool stun_specified_;
	bool is_running_;
};

#endif //#ifndef __ASYNC_STUN_OBJ_H__
