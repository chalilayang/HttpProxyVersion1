#pragma once

#include "resource.h"
#include "sh_p2p_system_define.h"
#include <winsock2.h>
int get_register_id()
{
	DWORD dwClientID = 0;
	HKEY hKey = NULL;
	LONG nResult = ::RegOpenKeyEx(HKEY_CURRENT_USER, _T("SOFTWARE\\SOHU\\Winacc"), 0, KEY_READ, &hKey);
	if (nResult == ERROR_SUCCESS)
	{
		DWORD dwLen = sizeof(DWORD), dwType = 0;
		nResult = ::RegQueryValueEx(hKey, _T("RegisterId"), NULL, &dwType, (LPBYTE)&dwClientID, &dwLen);
		::RegCloseKey(hKey);
	}
	return dwClientID;
}

char* GetLocalIp()
{
	char host_name[32] = "";
	int localip = 0;
	if(gethostname(host_name,sizeof(host_name)) == 0)
	{
		hostent *hostInfo = gethostbyname(host_name);	
		int index = 0;
		if(hostInfo && hostInfo->h_addr_list)
		{
			while (hostInfo->h_addr_list[index])
			{
				if(*(int *)hostInfo->h_addr_list[index] != inet_addr("127.0.0.1"))
				{
					localip = *(int *)hostInfo->h_addr_list[index];
					break;
				}
				++index;
			}
		}
	}

	return inet_ntoa(*(in_addr *)&localip);
}

void SHP2P_CALLBACK receive_data(sh_int_64 unique_id,sh_uint_32 index,sh_int_8_p data,sh_uint_32 len, bool isheader, sh_int_32 real_mp4_size)
{

};

void SHP2P_CALLBACK video_finish(sh_int_64 unique_id)
{

}

void SHP2P_CALLBACK register_notify(sh_int_32 id,sh_char_p sohu_key)
{
	//×¢²áIDÐ´µ½×¢²á±í
	HKEY hKey = NULL;
	if(::RegOpenKey(HKEY_CURRENT_USER,_T("SOFTWARE\\SOHU\\Winacc"),&hKey) == ERROR_SUCCESS)
	{
		DWORD	dwType = REG_DWORD;
		::RegSetValueEx(hKey,_T("RegisterId"),NULL,dwType,(LPBYTE)&id,sizeof(id));
		RegCloseKey(hKey);
	}
	else
	{
		DWORD ret =RegCreateKeyEx(HKEY_CURRENT_USER, _T("SOFTWARE\\SOHU\\Winacc"), 0, 0, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, NULL);
		if (ret == ERROR_SUCCESS)
		{
			DWORD dwSize=sizeof(DWORD);
			DWORD dwType=REG_DWORD;
			::RegSetValueEx(hKey,_T("RegisterId"),NULL,dwType,(LPBYTE)&id,sizeof(id));
			RegCloseKey(hKey);
		}
	}
}