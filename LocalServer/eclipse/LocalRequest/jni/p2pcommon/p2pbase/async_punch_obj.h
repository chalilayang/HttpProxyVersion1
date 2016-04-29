#ifndef __ASYNC_PUNCH_OBJ_H__
#define __ASYNC_PUNCH_OBJ_H__

#include "udp_def.h"
#include "punch_wrapper.h"
#include "../base/ioservice_pool.h"
#include "../base/common.h"
#include "../base/timer.h"

////////////////////////////////Constant Definition//////////////////////////////////////////
const int32_t kUDPMTUSize = 1500;
const int32_t kRandomSeqNoBase = 10 * 1024;
const int32_t kGuessPortCount = 5;
const int32_t kPacketCountSendPerTime = 2;

//////////////////////////////////AsyncPunchObjBase////////////////////////////////////////
class AsyncPunchObjBase : public boost::enable_shared_from_this<AsyncPunchObjBase>
{
public:
    AsyncPunchObjBase(
        boost::asio::io_service &ios,
		boost::shared_ptr<boost::asio::ip::udp::socket> socket_ptr,
        const SHPunchParam &param,
        const SH_PUNCH_CALLBACK callback);
    virtual ~AsyncPunchObjBase();

public:
    bool punch();
    bool stop();

protected:
    virtual bool send_packet(SHUdpCmdType packet_type,uint32_t target_ip,uint32_t target_port);
    virtual bool reply_packet(SHUdpCmdType packet_type);
    virtual bool do_punch() = 0;
    virtual std::string to_string();

private:
    void active();
    bool async_rcv();
    void reset_buffer();
    bool handle_rcv_packet(const boost::system::error_code &error,std::size_t bytes_transferred);
    bool handle_punch_request();
    bool handle_punch_response();
    bool handle_punch_response_response();
    bool handle_send_packet(const boost::system::error_code &error,std::size_t bytes_transferred);
    bool handle_timeout();
    bool on_punch_failed();
    bool on_punch_success();
    void print_errno(const std::string &function_name);
    
protected:
    boost::shared_ptr<boost::asio::ip::udp::socket> socket_;
    SHPunchParam punch_param_;
    SH_PUNCH_CALLBACK callback_;
    boost::asio::ip::udp::endpoint rcv_end_point_;
    char rcv_buffer_[kUDPMTUSize];
    MillisecTimer spend_time_;
    AsyncWaitTimer::Ptr punch_timer_;
    int32_t timeout_counter_;
    int32_t packet_count_per_time_;
    std::string punch_info_;
    bool rcv_flag_;
};

/////////////////////////////////////AsyncPunchImp1/////////////////////////////////////
class AsyncPunchImp1 : public AsyncPunchObjBase
{
public:
    typedef boost::shared_ptr<AsyncPunchImp1> Ptr;
    static Ptr create(
        boost::asio::io_service &ios,
		boost::shared_ptr<boost::asio::ip::udp::socket> socket_ptr,
        const SHPunchParam &param,
        const SH_PUNCH_CALLBACK callback);
    virtual ~AsyncPunchImp1();

private:
    AsyncPunchImp1(
        boost::asio::io_service &ios,
		boost::shared_ptr<boost::asio::ip::udp::socket> socket_ptr,
        const SHPunchParam &param,
        const SH_PUNCH_CALLBACK callback);
    virtual bool do_punch();
};

/////////////////////////////////////AsyncPunchImp2/////////////////////////////////////
class AsyncPunchImp2 : public AsyncPunchObjBase
{
public:
    typedef boost::shared_ptr<AsyncPunchImp2> Ptr;
    static Ptr create(
        boost::asio::io_service &ios,
		boost::shared_ptr<boost::asio::ip::udp::socket> socket_ptr,
        const SHPunchParam &param,
        const SH_PUNCH_CALLBACK callback);
    virtual ~AsyncPunchImp2();

private:
    AsyncPunchImp2(
        boost::asio::io_service &ios,
		boost::shared_ptr<boost::asio::ip::udp::socket> socket_ptr,
        const SHPunchParam &param,
        const SH_PUNCH_CALLBACK callback);
    virtual bool do_punch();
};

/////////////////////////////////////AsyncPunchImp3/////////////////////////////////////
class AsyncPunchImp3 : public AsyncPunchObjBase
{
public:
    typedef boost::shared_ptr<AsyncPunchImp3> Ptr;
    static Ptr create(
        boost::asio::io_service &ios,
		boost::shared_ptr<boost::asio::ip::udp::socket> socket_ptr,
        const SHPunchParam &param,
        const SH_PUNCH_CALLBACK callback);
    virtual ~AsyncPunchImp3();

private:
    AsyncPunchImp3(
        boost::asio::io_service &ios,
		boost::shared_ptr<boost::asio::ip::udp::socket> socket_ptr,
        const SHPunchParam &param,
        const SH_PUNCH_CALLBACK callback);
    virtual bool do_punch();
};

#endif //#ifndef __ASYNC_PUNCH_OBJ_H__
