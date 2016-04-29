#ifndef _NAT_DETECT_H_
#define _NAT_DETECT_H_
#include "udp_def.h"
#include "../base/common.h"
#include "../base/socket_api.h"

using boost::asio::ip::udp;

class NatDetect
{
public:
    NatDetect();
    ~NatDetect();
    SHNatType get_nat_type();
    bool get_map_addr(SOCKET map_socket,uint32_t& map_ip,uint16_t& map_port);
private:
    bool send_recv_msg(SOCKET sock, SOCKADDR * addr, uint32_t* addrlen, bool is_change_ip, bool is_chage_port, std::string& map_ip, uint16_t& map_port, std::string& str_change_ip, uint16_t& change_port);
    bool send_recv_msg(SOCKET sock, SOCKADDR * addr, uint32_t* addrlen, std::string& map_ip, uint16_t& map_port);
private:
    std::string stun_server_host_;
    uint16_t stun_server_port_;
    udp::endpoint stun_endpoint_;
};
#endif