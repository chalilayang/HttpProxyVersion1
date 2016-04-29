#ifndef __UTILS_H__
#define __UTILS_H__
#include <string>
#include <vector>
#include "../p2pcommon/base/common.h"

void trim_str(std::wstring& data, const std::wstring& str = L" ");
void trim_str(std::string& data, const std::string& str = " ");

void trim_left(std::wstring& data, const std::wstring& str = L" ");
void trim_left(std::string& data, const std::string& str = " ");

void trim_right(std::wstring& data, const std::wstring& str = L" ");
void trim_right(std::string& data, const std::string& str = " ");

std::vector<std::wstring> SpliterString(const std::wstring& data, const std::wstring& spliter);
std::vector<std::string>  SpliterString(const std::string& data, const std::string& spliter);

#endif  //__UTILS_H__


#if 0
#pragma once
#include "string"
#include "list"
#include "vector"
using namespace std;

namespace UtilBase64
{
	std::string convert_to_base64( const std::string& buf );
	std::string convert_from_base64(const std::string& base64);
}
//
wstring Utf82W(LPCSTR szContent,int size = -1);
string  W2Utf8(LPCWSTR szContent,int size = -1);
wstring ANSIToUnicode(LPCSTR szContent,int size = -1);
wstring GBKToUnicode(LPCSTR szContent,int size = -1);
//
void trace(wchar_t *fmt,...);
//
std::pair<wstring,wstring> ParseHeader(const wstring& header);
wstring urldecode(wstring& url,bool bUtf8 = true);
wstring	urlencode(wstring& url,bool bUtf8 = true);
wstring urldecode(const wstring& url,bool bUtf8 = true);
wstring	urlencode(const wstring& url,bool bUtf8 = true);
//
wstring hash_to_string(const string& hashin);
string  hash_from_string(const wstring& hashstr);
//
wstring GetMaxFreeDisk(__int64& capacity);
__int64 GetFreeCapacity(const wstring& disk);
//
DWORD	GetVersionNumber(LPCTSTR szFilePath);
wstring GetVersionString(DWORD dwVersion);
//
int		GetHostLocalIp();
//
BOOL	CopyText(HWND hWnd,LPCTSTR szText);
//
wstring GetUuid();

void	WaitForThreadExit(HANDLE hThread,DWORD timeOut = 3000);
//设置线程名字，szThreadName不能超过8个字符
void	SetThreadName(DWORD dwThreadID, LPCSTR szThreadName);
wstring GetPersonalDir();
wstring GetCurrentProcessPersonalDir();
//
int		CheckSoftState(LPCTSTR softname);
//
vector<DWORD> GetProcessList(LPCTSTR name);
BOOL	KillProcess(DWORD dwProcessID);
//
string	GetMachineCode();
BOOL	GetDesktopPath(wstring &strPath);   //桌面路径
//XXTEA 加密
void btea(UINT32 *v, int n, UINT32 const key[4]);


wstring OS_GetUserName();
wstring OS_GetSysInstallTime();
wstring AddSlash(LPCTSTR szStr);
#ifdef _DEBUG
#define SHTRACE  trace
#else
#define SHTRACE  (void)
#endif
#endif
