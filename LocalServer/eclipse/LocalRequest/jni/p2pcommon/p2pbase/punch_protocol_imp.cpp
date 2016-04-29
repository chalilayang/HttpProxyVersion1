#include "punch_protocol_imp.h"

PunchProtocolImp::PunchProtocolImp()
{

}

PunchProtocolImp::~PunchProtocolImp()
{

}

bool PunchProtocolImp::encode_punch(SHUdpCmdType type,char *buffer,int &length)
{
    bool ret = true;
    do 
    {
        int32_t packet_len = sizeof(SHUdpHeader);
        if (buffer == NULL || length < packet_len)
        {
            ret = false;
            length = packet_len;
            break;
        }

        //Using host byte order(litter endian)
        SHUdpHeader packet;
        packet.len = packet_len;
        packet.cmd = type;

        memcpy(buffer,&packet,packet_len);
        length = packet_len;
    } while (0);
    return ret;
}

bool PunchProtocolImp::decode_punch(const char *buffer,int length,SHUdpHeader &packet)
{
    bool ret = true;
    do 
    {
        int32_t struct_length = sizeof(SHUdpHeader);
        if (buffer == NULL || length < struct_length)
        {
            ret = false;
            break;
        }

        int32_t packet_length = *((int32_t*)(buffer));
        if (length < packet_length)
        {
            ret = false;
            break;
        }

        //Using host byte order(litter endian)
        int32_t packet_cmd = *((int32_t*)(buffer + sizeof(int32_t)));
        packet.len = packet_length;
        packet.cmd = packet_cmd;
        packet.encrypt = 0;
    } while (0);
    return ret;
}

bool PunchProtocolImp::encode_binding(int16_t type,char *buffer,int &length,uint32_t transaction_id,bool change_ip,bool change_port)
{
    bool ret = true;
    do 
    {
        int32_t header_size = (int32_t)sizeof(_StunHeader);
        if (buffer == NULL || length < header_size)
        {
            length = header_size;
            ret = false;
            break;
        }

        _StunHeader header = {0};
        header.type = htons(type);
        int16_t attri_size = sizeof(_StunAttriChangeIpRequst);
        header.length = (change_ip || change_port) ? htons(attri_size) : 0;

		if (transaction_id > 0)
		{
			memset(header.id,0,sizeof(header.id));
			memcpy(header.id,&transaction_id,sizeof(transaction_id));
		}

        memcpy(buffer,(char*)&header,header_size);
        length = header_size;

        if(change_ip || change_port)
        {
            _StunAttriChangeIpRequst attri = {0};
            memset(&attri,0,attri_size);
            attri.header.length = htons(4);
            attri.header.type = htons(kChangeRequest);
            if(change_ip)
                attri.value |= 0x4;
            if(change_port)
                attri.value |= 0x2;
            attri.value = htonl(attri.value);

            memcpy(buffer + length,(char*)&attri,attri_size);
            length += attri_size;
        }
    } while (0);
    return ret;
}

bool PunchProtocolImp::decode_binding_rsp(
    const char *buffer,
    int length,
    uint32_t& transaction_id,
    uint32_t& mapped_ip, 
    uint16_t& mapped_port,
    uint32_t& source_ip, 
    uint16_t& source_port,
    uint32_t& changed_ip, 
    uint16_t& changed_port)
{
    bool ret = true;
    do 
    {
        int32_t header_size = (int32_t)sizeof(_StunHeader);
        if (buffer == NULL || length < header_size)
        {
            ret = false;
            break;
        }

        int16_t packet_type = ntohs(*((int16_t*)(buffer)));
        int16_t packet_length = ntohs(*((int16_t*)(buffer + sizeof(int16_t))));
		std::string trans_id_str = std::string(buffer + sizeof(int16_t) + sizeof(int16_t),sizeof(transaction_id));
        transaction_id = *((uint32_t*)trans_id_str.c_str());

        if (packet_type != kBindResponseMsg)
        {
            ret = false;
            break;
        }

        if (length < packet_length)
        {
            ret = false;
            break;
        }

        const char *begin = buffer + sizeof(_StunHeader);
        length -= sizeof(_StunHeader);
        const char *end = begin + length;

        while (begin != end)
        {
            _StunAttriAddr attr = {0};
            switch(get_stun_address_type(begin))
            {
            case kMappedAddress:
                {
                    begin = get_stun_address_obj(begin,attr);
                    mapped_ip = attr.addr;
                    mapped_port = attr.port;
                    break;
                }
            case kSourceAddress:
                {
                    begin = get_stun_address_obj(begin,attr);
                    source_ip = attr.addr;
                    source_port = attr.port;
                    break;
                }
            case kChangedAddress:
                {
                    begin = get_stun_address_obj(begin,attr);
                    changed_ip = attr.addr;
                    changed_port = attr.port;
                    break;
                }
            default:
                break;
            }
        }
    } while (0);
    return ret;
}

int16_t PunchProtocolImp::get_stun_address_type(const char *p)
{
    int16_t type = ntohs(*((int16_t*)(p)));
    return type;
}

const char *PunchProtocolImp::get_stun_address_obj(const char *p,_StunAttriAddr &address_obj)
{
    memcpy(&address_obj,p,sizeof(address_obj));
    const char *next_attr = p + sizeof(address_obj);
    return next_attr;
}
