#ifndef _ALGORITHM_H__
#define _ALGORITHM_H__

#include "common.h"

#include <string>
#include <memory>
#ifdef WIN32
#pragma comment(lib, "ole32.lib")
#else
#include <sys/time.h>
#include "socket_api.h"
#endif 

inline std::string w2b(const std::tstring& _src)
{
#ifdef WIN32
	int nBufSize = ::WideCharToMultiByte(GetACP(), 0, _src.c_str(),-1, NULL, 0, 0, FALSE);

	char *szBuf = new char[nBufSize + 1];

	::WideCharToMultiByte(GetACP(), 0, _src.c_str(),-1, szBuf, nBufSize, 0, FALSE);

	std::string strRet(szBuf);

	delete []szBuf;
	szBuf = NULL;

	return strRet;
#else
  return _src;
#endif
}

inline std::tstring b2w(const std::string& _src)
{
#ifdef WIN32
	//计算字符串 string 转成 wchar_t 之后占用的内存字节数
	int nBufSize = ::MultiByteToWideChar(GetACP(),0,_src.c_str(),-1,NULL,0); 

	//为 wsbuf 分配内存 BufSize 个字节
	wchar_t *wsBuf = new wchar_t[nBufSize + 1];

	//转化为 unicode 的 WideString
	::MultiByteToWideChar(GetACP(),0,_src.c_str(),-1,wsBuf,nBufSize); 

	std::tstring wstrRet(wsBuf);

	delete []wsBuf;
	wsBuf = NULL;

	return wstrRet;
#else
  return _src;
#endif
}

inline std::string Wide2Utf8(const std::tstring& _src)
{
#ifdef WIN32
	int nBufSize = ::WideCharToMultiByte(CP_UTF8, 0, _src.c_str(),-1, NULL, 0, NULL, NULL);

	char *szBuf = new char[nBufSize + 1];

	::WideCharToMultiByte(CP_UTF8, 0, _src.c_str(),-1, szBuf, nBufSize, NULL, NULL);

	std::string strRet(szBuf);

	delete []szBuf;
	szBuf = NULL;

	return strRet;
#else
  return _src;
#endif
}

inline std::tstring Utf82Wide(const std::string& _src)
{
#ifdef WIN32
	//计算字符串 string 转成 wchar_t 之后占用的内存字节数
	int nBufSize = ::MultiByteToWideChar(CP_UTF8,0,_src.c_str(),-1,NULL,0); 

	//为 wsbuf 分配内存 BufSize 个字节
	wchar_t *wsBuf = new wchar_t[nBufSize + 1];

	//转化为 unicode 的 WideString
	::MultiByteToWideChar(CP_UTF8,0,_src.c_str(),-1,wsBuf,nBufSize); 

	std::tstring wstrRet(wsBuf);

	delete []wsBuf;
	wsBuf = NULL;

	return wstrRet;
#else
  return _src;
#endif
}

inline std::tstring hash_to_string(const std::string& hashin)
{
	static const char* hashbase64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";
	std::string hashout;
	char  *pHash = (char *)hashin.data();
	int   nShift = 7;

	int nchar;
	for (nchar = 32 ; nchar ; nchar-- )
	{
		uint8_t nBits = 0;
		int  nbit;
		for ( nbit = 0 ; nbit < 5 ; nbit++ )
		{
			if ( nbit ) nBits <<= 1;
			nBits |= ( *pHash >> nShift ) & 1;

			if ( ! nShift-- )
			{
				nShift = 7;
				pHash++;
			}
		}
		hashout.push_back(hashbase64[nBits]);
	}
	return b2w(hashout.c_str());
}

inline std::string hash_from_string(const std::tstring& hashstr)
{
	int	    nchars	= 0;
	int		nbits   = 0;
	int		ncount  = 0;
	std::string  hashout;
	if ( hashstr.size() < 32 )
		return hashout;
	std::string  ahash   = w2b(hashstr);
	unsigned char*	stringin =  (unsigned char*)ahash.data();
	for (nchars = 32 ; nchars-- ; stringin++ )
	{
		if ( *stringin >= 'A' && *stringin <= 'Z' )
		{
			nbits |= ( *stringin - 'A' );
		}
		else if ( *stringin >= 'a' && *stringin <= 'z' )
		{
			nbits |= ( *stringin - 'a' );
		}
		else if ( *stringin >= '2' && *stringin <= '7' )
		{
			nbits |= ( *stringin - '2' + 26 );
		}
		else
		{
			return hashout;
		}
		ncount += 5;
		if ( ncount >= 8 )
		{
			hashout.push_back((unsigned char)(nbits >> (ncount - 8)));
			ncount -= 8;
		}
		nbits <<= 5;
	}
	return hashout;
}


inline void Splite(std::vector<std::string>& result, const std::string& input, const std::string& spliter)
{
    std::string::size_type spliter_length = spliter.size();
    std::string::size_type last_pos = 0;

    while(last_pos < input.length())
    {
        std::string::size_type pos = input.find(spliter, last_pos);
        if( pos == std::string::npos )
        {
            result.push_back(input.substr(last_pos));
            return;
        }
        result.push_back(input.substr(last_pos, pos - last_pos));
        last_pos = pos + spliter_length;
    }

}

inline bool is_digit2(std::string &instr) 
{ 
    for(size_t i=0;i<instr.size();i++) 
    {   
        if ((instr.at(i)>'9') || (instr.at(i)<'0'))
        { 
            return false;		 
        }
    } 
    return true; 
}

inline void url_path_remove(std::string& url, const std::string& param)
{
    std::string::size_type pos_start = url.find(param);

    if (pos_start != std::string::npos && pos_start != 0 && pos_start != url.size()-1)
    {
        std::string::size_type pos_end = url.find("&", pos_start+param.size());
        if (pos_end == std::string::npos)
        {
            url.resize(pos_start);
        }
        else
        {
            std::string url_left = url.substr(0, pos_start);
            url_left.append(url.substr(pos_end+1));
            url = url_left;
        }
    }
}

inline uint32_t IpToUint(const char* ip) {
#ifdef WIN32
    return (uint32_t)inet_addr(ip);
#else
    struct in_addr addr;
    if ( 0 != inet_aton(ip, &addr) )
    {
        return addr.s_addr;
    }
    return 0;
#endif
}

inline uint32_t ip2uint(const std::string &ip_s)
{
	uint32_t num[4];
	int i;
	size_t pos_start = 0,pos;

	for(i=0;i<4;i++)
	{
		pos = ip_s.find('.',pos_start);
		std::string one;
		if(pos == std::string::npos)
		{
			one = ip_s.substr(pos_start);
		}
		else
		{
			one = ip_s.substr(pos_start,pos-pos_start);
			pos_start = pos + 1;
		}
		num[i] = atoi(one.c_str());
	}
	if(i==4)
	{
		uint32_t ip = 0;
		ip += num[0]<<24;
		ip += num[1]<<16;
		ip += num[2]<<8;
		ip += num[3];
		return ip;
	}
	else
	{
		return 0;
	}
}

//输入本地字节序
inline std::string uint2ip(uint32_t ip_uint)
{
#ifdef WIN32
    struct in_addr  addr;
    addr.S_un.S_addr = ip_uint;
    return std::string(::inet_ntoa(addr));
#else
    char ip_buff[32];
    ::inet_ntop(AF_INET, &ip_uint, ip_buff, 32);
    return std::string(ip_buff);
#endif
}
#endif
