#ifndef __PUNCH_PROTOCOL_IMP_H__
#define __PUNCH_PROTOCOL_IMP_H__

#include "udp_def.h"
#include "async_stun_obj.h"

#pragma pack(push,1)
typedef struct _StunHeader
{
    short	type;
    short	length;
    char	id[16];
}_StunHeader;

typedef struct _StunAttriHeader
{
    short	type;
    short	length;
}_StunAttriHeader;

typedef struct _StunAttriAddr
{
    _StunAttriHeader header;
    short	family;
    short	port;
    int		addr;
}_StunAttriAddr;

typedef struct _StunAttriChangeIpRequst
{
    _StunAttriHeader header;
    int				value;
}_StunAttriChangeIpRequst;

typedef struct _WRAPHEADER
{
    unsigned short wFlag;
    unsigned short wLen;

} _WRAPHEADER, *_PWRAPHEADER ;

#pragma pack(pop)

const int16_t kBindRequestMsg   = 0x0001;
const int16_t kBindResponseMsg  = 0x0101;
const int16_t kMappedAddress    = 0x0001;
const int16_t kSourceAddress    = 0x0004;
const int16_t kChangedAddress   = 0x0005;
const int16_t kChangeRequest	= 0x0003;

class PunchProtocolImp
{
public:
    PunchProtocolImp();
    ~PunchProtocolImp();

public:
    static bool encode_punch(SHUdpCmdType type,char *buffer,int &length);
    static bool decode_punch(const char *buffer,int length,SHUdpHeader &packet);
    static bool encode_binding(int16_t type,char *buffer,int &length,uint32_t transaction_id = 0,bool change_ip = false,bool change_port = false);
    static bool decode_binding_rsp(
        const char *buffer,
        int length,
        uint32_t& transaction_id,
        uint32_t& mapped_ip, 
        uint16_t& mapped_port,
        uint32_t& source_ip, 
        uint16_t& source_port,
        uint32_t& changed_ip, 
        uint16_t& changed_port);

private:
    static int16_t get_stun_address_type(const char *p);
    static const char *get_stun_address_obj(const char *p,_StunAttriAddr &address_obj);
};

#endif //#ifndef __PUNCH_PROTOCOL_H__
