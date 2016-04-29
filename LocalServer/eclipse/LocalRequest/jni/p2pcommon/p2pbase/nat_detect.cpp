#include "nat_detect.h"
#include "../base/socket_api.h"
#include "../base/algorithm.h"
#include "../sh_kernel.h"
#ifdef ANDROID
#include "ifaddrs.h"
#else

#ifdef WIN32
#else
#include <ifaddrs.h>
#endif

#endif

#pragma pack(push,1)
typedef struct StunHeader
{
    short	type;
    short	length;
    char	id[16];
}StunHeader;

typedef struct StunAttriHeader
{
    short	type;
    short	length;
}StunAttriHeader;

typedef struct StunAttriAddr
{
    StunAttriHeader header;
    short	family;
    short	port;
    int		addr;
}StunAttriAddr;

typedef struct StunAttriChangeIpRequst
{
    StunAttriHeader header;
    int				value;
}StunAttriChangeIpRequst;

typedef struct WRAPHEADER
{
    unsigned short wFlag;
    unsigned short wLen;

} WRAPHEADER, *PWRAPHEADER ;

#pragma pack(pop)
const short BindRequestMsg               = 0x0001;
const short BindResponseMsg              = 0x0101;

#define HDRFLAG1	0x5858
#define HDRFLAG2	0xa6a6

const short MappedAddress		= 0x0001;
const short ResponseAddress		= 0x0002;
const short ChangeRequest		= 0x0003;
const short SourceAddress		= 0x0004;
const short ChangedAddress		= 0x0005;
const short Username			= 0x0006;
const short Password			= 0x0007;
const short MessageIntegrity	= 0x0008;
const short ErrorCode			= 0x0009;
const short UnknownAttribute	= 0x000A;
const short ReflectedFrom		= 0x000B;
const short XorMappedAddress	= 0x0020;
const short XorOnly				= 0x0021;
const short ServerName			= 0x0022;
const short SecondaryAddress	= 0x0050; // Non standard extention
const short CHANNEL_NUMBER		= 0x000C;
const short LIFETIME		    = 0x000D;
const short Reserved		    = 0x0010;
const short XOR_PEER_ADDRESS    = 0x0012;
const short DATA			    = 0x0013;
const short XOR_RELAYED_ADDRESS =0x0016;
const short EVEN_PORT		    = 0x0018;
const short REQUESTED_TRANSPORT =0x0019;
const short DONT_FRAGMENT		= 0x001A;
const short RESERVATION_TOKEN	= 0x0022;

short get_msg_type(const char* msg);
char* get_addr(char* msg,StunAttriAddr& atti);
int wrap(char *out_buf ,char *in_buf, unsigned short winlen);
int unwrap(char **ppout_buf, char *in_buf, uint16_t winlen);
void parse_msg(char* msg, uint32_t len, std::string& map_ip, uint16_t& map_port, std::string& change_ip, uint16_t& change_port);
void send_message(SOCKET sock,char* msg,uint32_t len,sockaddr* addr);
std::string recv_message(SOCKET sock,sockaddr* addr,uint32_t* addrlen);
void    get_change_ip_requst_msg(StunAttriChangeIpRequst& msg,bool is_change_ip,bool is_change_port);
std::vector<std::string> get_host_ip();

NatDetect::NatDetect():stun_server_port_(3478),stun_server_host_("stun.p2p.hd.sohu.com")
{
    //获取新io_server
    boost::asio::io_service& io_service = server_ios();
    udp::resolver resolver(io_service);
    udp::resolver::query query(stun_server_host_, "80");
    boost::system::error_code error;
    udp::resolver::iterator itr = resolver.resolve(query, error);
    
    if (itr != udp::resolver::iterator())
    {
        stun_endpoint_ = *itr;
    }
}

NatDetect::~NatDetect()
{

}

SHNatType NatDetect::get_nat_type()
{
    //DWORD startTime = get_tickcount();
    SHNatType nat_type = NatType_ERROR;
    SOCKET nat_detect_s = SocketAPI::socket_ex(AF_INET,SOCK_DGRAM,0);
    SOCKADDR_IN addr;
    uint32_t addr_len = szSOCKADDR_IN;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(stun_server_port_);
#ifdef WIN32
    addr.sin_addr.S_un.S_addr = inet_addr(stun_endpoint_.address().to_string().c_str());
#else
    addr.sin_addr.s_addr = inet_addr(stun_endpoint_.address().to_string().c_str());
#endif
    
    std::string map_ip;
    uint16_t map_port = 0;
    std::string change_ip;
    uint16_t change_port = 0;
    vector<std::string> ips = get_host_ip();
    std::string stun_ip2;
    uint16_t stun_port2 = 0;

    if (!send_recv_msg(nat_detect_s, (SOCKADDR *)&addr, &addr_len, false, false, map_ip, map_port, change_ip, change_port))
    {
        nat_type = NatType_BLOCKED;
        goto end_pro;
    }

    stun_ip2 = change_ip;
    stun_port2 = change_port;

    if (std::find(ips.begin(), ips.end(), map_ip) != ips.end() )
    {
        addr.sin_family = AF_INET;
        addr.sin_port = htons(stun_server_port_);
#ifdef WIN32
        addr.sin_addr.S_un.S_addr = inet_addr(stun_endpoint_.address().to_string().c_str());
#else
        addr.sin_addr.s_addr = inet_addr(stun_endpoint_.address().to_string().c_str());
#endif
        addr_len = szSOCKADDR_IN;
        
        if (!send_recv_msg(nat_detect_s, (SOCKADDR *)&addr, &addr_len, true, true, map_ip, map_port, change_ip, change_port))
        {
            nat_type = NatType_FIREWALL;
            goto end_pro;
        }
        else
        {
            nat_type = NatType_OPEN_INTERNET;
            goto end_pro;
        }
    }
    else
    {
        addr.sin_family = AF_INET;
        addr.sin_port = htons(stun_server_port_);
#ifdef WIN32
        addr.sin_addr.S_un.S_addr = inet_addr(stun_endpoint_.address().to_string().c_str());
#else
        addr.sin_addr.s_addr = inet_addr(stun_endpoint_.address().to_string().c_str());
#endif
        addr_len = szSOCKADDR_IN;
        
        if(send_recv_msg(nat_detect_s, (SOCKADDR *)&addr, &addr_len, true, true, map_ip, map_port, change_ip, change_port))
        {
            nat_type = NatType_FULL_CONE;
            goto end_pro;
        }
        else
        {

        }
        //测试是否对称型
        addr.sin_family = AF_INET;
        addr.sin_port = htons(stun_port2);
#ifdef WIN32
        addr.sin_addr.S_un.S_addr = inet_addr(stun_ip2.c_str());
#else
        addr.sin_addr.s_addr = inet_addr(stun_ip2.c_str());
#endif
        addr_len = szSOCKADDR_IN;
        std::string map_ip1;
        uint16_t map_port1 = 0;
        send_recv_msg(nat_detect_s, (SOCKADDR *)&addr,&addr_len,false,false,map_ip1,map_port1, change_ip, change_port);
        if(map_ip1 != map_ip || map_port1 != map_port)
        {
            //对称型，一般不能通信
            nat_type = NatType_SYMMETRIC_NAT;
            goto end_pro;
        }
        else
        {
    
        }
        //测试是否限制Port
        addr.sin_family = AF_INET;
        addr.sin_port = htons(stun_server_port_);
#ifdef WIN32
        addr.sin_addr.S_un.S_addr = inet_addr(stun_endpoint_.address().to_string().c_str());
#else
        addr.sin_addr.s_addr = inet_addr(stun_endpoint_.address().to_string().c_str());
#endif
        addr_len = szSOCKADDR_IN;
        if(send_recv_msg(nat_detect_s, (SOCKADDR *)&addr, &addr_len, false, true, map_ip, map_port, change_ip, change_port))
        {
            nat_type = NatType_RESTRICTED;
            goto end_pro;
        }
        else
        {
            nat_type = NatType_PORT_RESTRICTED;
            goto end_pro;
        }
    }

end_pro:
   //UDPTOOL_TRACE(_T("detect time %dms\r\n"),GetTickCount()-startTime);
    if (nat_detect_s > 0)
    {
        SocketAPI::closesocket_ex(nat_detect_s);
    }
    return nat_type;
}

bool NatDetect::get_map_addr(SOCKET map_socket, uint32_t& map_ip,uint16_t& map_port )
{

    SOCKADDR_IN addr;
    uint32_t addr_len = szSOCKADDR_IN;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(stun_server_port_);
#ifdef WIN32
    addr.sin_addr.S_un.S_addr = inet_addr(stun_endpoint_.address().to_string().c_str());
#else
    addr.sin_addr.s_addr = inet_addr(stun_endpoint_.address().to_string().c_str());
#endif
    std::string mapIp;
    uint16_t mapPort = 0;
    //
    if(!send_recv_msg(map_socket,(sockaddr *)&addr,&addr_len,mapIp,mapPort))
    {
        return false;
    }
    else
    {
        map_ip = inet_addr(mapIp.c_str());
        map_port= htons(mapPort);
        return true;
    }
}

bool NatDetect::send_recv_msg( SOCKET sock,SOCKADDR* addr, uint32_t* addrlen, bool is_change_ip, bool is_chage_port, std::string& map_ip, uint16_t& map_port, std::string& str_change_ip, uint16_t& change_port )
{
    //send and recv udp package
    //DWORD startTime = get_tickcount();
    //GUID   guid; 
    //CoCreateGuid(&guid); //id没有使用，移植时不需要，header.id不用复制
    StunHeader header = {0};
    std::string str_msg;
    header.type = htons(BindRequestMsg);
    header.length = (is_change_ip || is_chage_port) ? htons(sizeof(StunAttriChangeIpRequst)) : 0;
    //memcpy(header.id,&guid,16);
    str_msg.append((char*)&header,sizeof(StunHeader));
    if(is_change_ip || is_chage_port)
    {
        StunAttriChangeIpRequst attri = {0};
        get_change_ip_requst_msg(attri,is_change_ip,is_chage_port);
        str_msg.append((char*)&attri,sizeof(StunAttriChangeIpRequst));
    }
    std::string recv_msg;
    uint32_t  total_time = 0;
    unsigned int opt_len = 0;
    
#ifdef WIN32
    uint32_t index = 0;
    uint32_t old_timeout = 0;
    uint32_t time_out = 100;
    uint32_t len = str_msg.size();
#else
    struct timeval old_timeout,time_out;
    time_out.tv_sec = 0;
    time_out.tv_usec = 100000;
#endif
    
    SocketAPI::getsockopt_ex(sock,SOL_SOCKET,SO_RCVTIMEO,(void *)&old_timeout,&opt_len);
    while (total_time < 1600)
    {
#ifdef WIN32
        if(time_out + total_time > 1600)
            time_out = 1600 - total_time;
#else
        if (time_out.tv_usec/1000 + total_time > 1600) {
            time_out.tv_usec = (1600 - total_time) * 1000;
        }
#endif
        bool succes = SocketAPI::setsockopt_ex(sock,SOL_SOCKET,SO_RCVTIMEO,(void*)&time_out,sizeof(time_out));
        send_message(sock, (char *)str_msg.data(), str_msg.size(), addr);
        //DWORD recvStartTime = get_tickcount();
        recv_msg = recv_message(sock, addr, addrlen);
        StunHeader recvheader	= {0};
        if(recv_msg.size() >= sizeof(StunHeader))
        {
            memcpy(&recvheader ,recv_msg.data(), sizeof(StunHeader));
            //判断是否收到了回应包
            if(memcmp(&recvheader.id,&header.id,sizeof(header.id)) == 0)
                break;
        }
        //UDPTOOL_TRACE(_T("recv msg elscap time = %d\r\n"),GetTickCount()-recvStartTime);
#ifdef WIN32
        total_time += time_out;
        time_out *= 2;
#else
        total_time += time_out.tv_usec/1000;
        time_out.tv_usec *= 2;
#endif
    }
    if(recv_msg.size() == 0)
    {
        //UDPTOOL_TRACE(_T("send and recv msg elscap time = %d\r\n"),GetTickCount()-startTime);
        return false;
    }
    parse_msg((char*)recv_msg.data(),recv_msg.size(),map_ip, map_port, str_change_ip, change_port);
    SocketAPI::setsockopt_ex(sock,SOL_SOCKET,SO_RCVTIMEO,(void *)&old_timeout, sizeof(old_timeout));
    //UDPTOOL_TRACE(_T("send and recv msg elscap time = %d\r\n"),GetTickCount()-startTime);
    return true;
}

bool NatDetect::send_recv_msg( SOCKET sock,SOCKADDR* addr, uint32_t* addrlen, std::string& map_ip, uint16_t& map_port )
{
#ifdef _DEBUG
    //DWORD startTime = GetTickCount();
#endif
    //GUID   guid; 
    //CoCreateGuid(&guid); 
    StunHeader header = {0};
    std::string str_msg;
    header.type = htons(BindRequestMsg);
    header.length = 0;
    //memcpy(header.id,&guid,16);
    str_msg.append((char*)&header,sizeof(header));

    std::string recv_msg;
    uint32_t total_time  = 0;
    unsigned int opt_len = 0;
#ifdef WIN32
    uint32_t index = 0;
    uint32_t old_timeout = 0;
    uint32_t time_out    = 100;
#else
    struct timeval old_timeout,time_out;
    time_out.tv_sec = 0;
    time_out.tv_usec = 100000;
#endif
    SocketAPI::getsockopt_ex(sock, SOL_SOCKET, SO_RCVTIMEO, (void*)&old_timeout, &opt_len);
    while (total_time < 1600)
    {
#ifdef WIN32
        if(time_out + total_time > 1600)
            time_out = 1600 - total_time;
#else
        if (time_out.tv_usec/1000 + total_time > 1600) {
            time_out.tv_usec = (1600 - total_time)*1000;
        }
#endif
        SocketAPI::setsockopt_ex(sock,SOL_SOCKET,SO_RCVTIMEO,(void*)&time_out,sizeof(time_out));
        send_message(sock,(char *)str_msg.data(), str_msg.size(), addr);
#ifdef _DEBUG
       // DWORD recvStartTime = GetTickCount();
#endif
        recv_msg = recv_message(sock,addr,addrlen);
        StunHeader recvheader = {0};
        if(recv_msg.size() >= sizeof(StunHeader))
        {
            memcpy(&recvheader, recv_msg.data(), sizeof(StunHeader));
            //判断是否收到了回应包
            if(memcmp(&recvheader.id,&header.id,sizeof(header.id)) == 0)
                break;
        }
#ifdef _DEBUG
        //UDPTOOL_TRACE(_T("recv msg elscap time = %d\r\n"),GetTickCount()-recvStartTime);
#endif
#ifdef WIN32
        total_time += time_out;
        time_out *= 2;
#else
        total_time += time_out.tv_usec/1000;
        time_out.tv_usec *= 2;
#endif
    }

    if(recv_msg.size() == 0)
    {
#ifdef _DEBUG
       // UDPTOOL_TRACE(_T("send and recv msg elscap time = %d\r\n"),GetTickCount()-startTime);
#endif
        return false;
    }
    std::string change_ip;
    uint16_t change_port;
    parse_msg((char*)recv_msg.data(), recv_msg.size(), map_ip, map_port, change_ip, change_port);
    SocketAPI::setsockopt_ex(sock,SOL_SOCKET,SO_RCVTIMEO,(void*)&old_timeout,sizeof(old_timeout));
#ifdef _DEBUG
    //UDPTOOL_TRACE(_T("send and recv msg elscap time = %d\r\n"),GetTickCount()-startTime);
#endif
    return true;
}


short get_msg_type( const char* msg )
{
    short type;
    memcpy(&type,msg,sizeof(short));
    return ntohs(type);
}

char* get_addr( char* msg,StunAttriAddr& atti )
{
    memcpy(&atti,msg,sizeof(atti));
    return msg + sizeof(atti);
}

int wrap( char *out_buf ,char *in_buf, unsigned short winlen )
{
    unsigned short wtotal = winlen + 6;
    PWRAPHEADER phdr = (PWRAPHEADER)out_buf;
    //设置flag
    phdr->wFlag = ~(wtotal | ((wtotal % 2) ? HDRFLAG1:HDRFLAG2 ));
    //设置长度
    phdr->wLen = ~wtotal;

    unsigned short wAux = 0;
    for (unsigned short i=0; i<winlen; i++)
    {
        *(out_buf +i+4) = ~*(in_buf +i);
        wAux += *(out_buf +i+4);
    }

    wAux += *(out_buf);
    wAux += *(out_buf+1);
    wAux += *(out_buf+2);
    wAux += *(out_buf+3);
    //设置校验和
    *(unsigned short *)(out_buf + wtotal - 2) = wAux | ((wtotal % 2) ? HDRFLAG1:HDRFLAG2 );
    return wtotal;
}

int unwrap( char **ppout_buf, char *in_buf, uint16_t winlen )
{
    unsigned short i;
    PWRAPHEADER phdr = (PWRAPHEADER)in_buf;

    //校验长度
    unsigned short wLen = ~phdr->wLen;
    if ( wLen != winlen )
        return 0;

    //校验标记
    unsigned short wAux = ~(wLen | ((wLen%2) ? HDRFLAG1 : HDRFLAG2));
    if ( phdr->wFlag != wAux )
        return 0;

    //校验和
    wAux = 0;
    for(i=0; i<winlen-2; i++)
        wAux += *(in_buf +i);

    wAux |= (wLen%2 ? HDRFLAG1 : HDRFLAG2);

    if ( wAux != *(unsigned short*)(in_buf+winlen-2) )
        return 0;

    for( i=4; i< winlen-2; i++ )
        *(in_buf+i) = ~*(in_buf+i);

    *ppout_buf = in_buf +4;
    return winlen-6;
}

void parse_msg( char* msg, uint32_t len, std::string& map_ip, uint16_t& map_port, std::string& change_ip, uint16_t& change_port )
{
    StunHeader stun_msg = {0};
    memcpy(&stun_msg,msg,sizeof(StunHeader));
    stun_msg.length = ntohs(stun_msg.length);
    stun_msg.type = ntohs(stun_msg.type);
    
    if(stun_msg.type != BindResponseMsg)
    {
        return;
    }
    //assert(stun_msg.type == BindResponseMsg);
    msg += sizeof(StunHeader);
    len -= sizeof(StunHeader);
    //
    char* szEnd = msg + len;
    char* szStart = msg;
    StunAttriAddr addrAtti = {0};
    map_port= 0;
    while (szStart != szEnd)
    {
        switch(get_msg_type(szStart))
        {
        case MappedAddress:
            {
                szStart = get_addr(szStart,addrAtti);
                in_addr inaddr;
#ifdef WIN32
                inaddr.S_un.S_addr = addrAtti.addr;
#else
                inaddr.s_addr = addrAtti.addr;
#endif
                map_ip = inet_ntoa(inaddr);
                map_port = ntohs(addrAtti.port);
                break;
            }
        case SourceAddress:
            {
                szStart = get_addr(szStart,addrAtti);
                in_addr inaddr;
#ifdef WIN32 
                inaddr.S_un.S_addr = addrAtti.addr;
#else
                inaddr.s_addr = addrAtti.addr;
#endif
                break;
            }
        case ChangedAddress:
            {
                szStart = get_addr(szStart,addrAtti);
                in_addr inaddr;
#ifdef WIN32 
                inaddr.S_un.S_addr = addrAtti.addr;
#else
                inaddr.s_addr = addrAtti.addr;
#endif
                change_ip = inet_ntoa(inaddr);
                change_port = ntohs(addrAtti.port);
                break;
            }
        default:
            return;
        }
    }
}

void send_message( SOCKET sock,char* msg,uint32_t len,sockaddr* addr )
{
    int sendlen = SocketAPI::sendto_ex(sock,msg,len,0,(sockaddr*)addr,sizeof(sockaddr_in));
}

std::string recv_message( SOCKET sock,sockaddr* addr,uint32_t* addrlen )
{
    std::string data;
    data.resize(2048+100);
    int recv_len = SocketAPI::recvfrom_ex(sock,(char*)data.data(),2048,0,addr, (unsigned int*)addrlen);
    if(recv_len > 0)
    {
        return std::string(data.data(), recv_len);
    }
    else
    {
        return "";
    }
}

void get_change_ip_requst_msg( StunAttriChangeIpRequst& msg,bool is_change_ip,bool is_change_port )
{
    memset(&msg,0,sizeof(msg));
    msg.header.length = htons(4);
    msg.header.type = htons(ChangeRequest);
    if(is_change_ip)
        msg.value |= 0x4;
    if(is_change_port)
        msg.value |= 0x2;
    msg.value = htonl(msg.value);
}

std::vector<std::string> get_host_ip()
{
    vector<std::string> ips;
#ifdef WIN32
    char host_name[260] = {0};
    if(gethostname(host_name,260) != 0)//#include <unistd.h>
        return ips;
    hostent* hostinfo = gethostbyname(host_name);//#include <netdb.h>
    for(size_t i = 0; i < hostinfo->h_length / sizeof(int); ++i)
    {
        ips.push_back(inet_ntoa(*(in_addr *)hostinfo->h_addr_list[i]));
    }
#else
    struct ifaddrs *interfaces = NULL;
    struct ifaddrs *temp_addr = NULL;
    int success = 0;
    
    // Retrieve the current interfaces - returns 0 on success
    success = getifaddrs(&interfaces);
    if (success == 0)
    {
        // Loop through linked list of interfaces
        temp_addr = interfaces;
        while (temp_addr != NULL)
        {
            if (((struct sockaddr_in *)temp_addr->ifa_addr) != NULL)
            {
                ips.push_back(inet_ntoa(((struct sockaddr_in *)temp_addr->ifa_addr)->sin_addr));
            }
            temp_addr = temp_addr->ifa_next;
        }
    }
    // Free memory
    freeifaddrs(interfaces);
#endif
    return ips;
}
