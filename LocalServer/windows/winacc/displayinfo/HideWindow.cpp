#include "stdafx.h"
#include "HideWindow.h"
#include <shellapi.h>

#define SH_DISP_DOWNLOADINFO	(WM_USER + 203)
#define SH_DISP_LOGINFO			(WM_USER + 204)

CHideWindow::CHideWindow(void)
{
	m_dispDialogPtr.reset(new CSHDispDialog());
	m_dispDialogPtr->Create();
	m_dispDialogPtr->ShowWindow(SW_HIDE);
}

CHideWindow::~CHideWindow(void)
{
	m_hWnd=NULL;
}

LRESULT CHideWindow::OnDestroy( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled )
{
	::UnregisterHotKey(m_hWnd,SH_DISP_DOWNLOADINFO);
	::UnregisterHotKey(m_hWnd,SH_DISP_LOGINFO);
	return 0;
}

LRESULT CHideWindow::OnCreate( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled )
{
	DefWindowProc( uMsg, wParam, lParam );
	return 1;
}

void CHideWindow::InitHotKey()
{
	::RegisterHotKey(m_hWnd,SH_DISP_DOWNLOADINFO,MOD_CONTROL|MOD_ALT,VK_F9);
	if(!::RegisterHotKey(m_hWnd,SH_DISP_LOGINFO,MOD_CONTROL|MOD_ALT,VK_F8))
	{
		ATL::CString strLog;
		strLog.Format(_T("×¢²ácontrol+alt+F11Ê§°Ü£¬´íÎóÂë%d,³¢ÊÔ×¢²ácontrol+shift+F11"),GetLastError());
		//INFO_LOG("kernel",strLog);
		if(!::RegisterHotKey(m_hWnd,SH_DISP_LOGINFO,MOD_CONTROL|MOD_SHIFT,VK_F11))
		{
			strLog.Format(_T("×¢²ácontrol+shift+F11Ê§°Ü£¬´íÎóÂë%d"),GetLastError());
			//INFO_LOG("kernel",strLog);
		}
	}	
}

LRESULT CHideWindow::OnHotKey( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled )
{
	int low  = (lParam & 0x00ff);
	int higt = lParam >> 16;
	if(higt == VK_F9 && low == (MOD_CONTROL|MOD_ALT) )
	{
		m_dispDialogPtr->ShowWindow(SW_SHOW);
	}
	else if((higt == VK_F8 && low == (MOD_CONTROL|MOD_ALT))|| (higt == VK_F8 && low == (MOD_CONTROL|MOD_SHIFT)))
	{
		DispLog();
	}
	return 1L;
}

void CHideWindow::DispLog()
{
	//::ShellExecute(NULL,_T("open"),get_log_path(),NULL,NULL,SW_SHOW);
}

