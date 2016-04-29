#include "stdafx.h"
#include "util.h"
#include <atlconv.h>
#include <atlstr.h>
std::wstring  NatTypeDesc(SHNatType type)
{
	wstring desc;
	switch (type)
	{
	case NatType_ERROR:
		desc = (L"δ̽��");
		break;
	case NatType_BLOCKED:
		desc = (L"Blocked");
		break;
	case NatType_OPEN_INTERNET:
		desc = (L"����");
		break;
	case NatType_FULL_CONE:
		desc = (L"ȫCONE");
		break;
	case NatType_PORT_RESTRICTED:
		desc = (L"�˿�����");
		break;
	case NatType_RESTRICTED:
		desc = (L"IP����");
		break;
	case NatType_FIREWALL:
		desc = (L"����ǽ");
		break;
	case NatType_SYMMETRIC_NAT:
		desc = (L"�Գ���");
		break;
	default:
		desc = (L"Unknown type");
		break;
	}
	return desc;
}

wstring GetIpStr(int ip)
{
	if(ip == 0)
	{
		return _T("0.0.0.0");
	}
	else
	{
		USES_CONVERSION;
		return A2W(inet_ntoa(*(in_addr *)&ip));
	}
}

wstring GetSizeDesc(int size)
{
	float fsize;
	ATL::CString strDesc = _T("B");
	strDesc.Format(_T("%.2f B"),(float)size);
	if((int)size / 1024 <= 0)
		return (LPCTSTR)strDesc;
	//
	fsize = (float)size / 1024; 
	strDesc.Format(_T("%.2f KB"),fsize);
	if((int)fsize / 1024 <= 0)
		return (LPCTSTR)strDesc;
	//
	fsize /= 1024; 
	strDesc.Format(_T("%.2f MB"),fsize);
	if((int)fsize / 1024 <= 0)
		return (LPCTSTR)strDesc;
	//
	fsize /= 1024; 
	strDesc.Format(_T("%.2f GB"),fsize);
	return (LPCTSTR)strDesc;
}
