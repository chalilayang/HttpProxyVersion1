#pragma once
#include <atlbase.h>
#include <atlwin.h>
#include "DispDialog.h"

class CHideWindow : public ATL::CWindowImpl<CHideWindow,CWindow,CFrameWinTraits>
{
public:
	DECLARE_WND_CLASS(_T("{BEAFADAA-2A20-46fb-BB0A-7FD54FED52CB}"))
	BEGIN_MSG_MAP(CHideWindow)
		MESSAGE_HANDLER(WM_CREATE,OnCreate)
		MESSAGE_HANDLER(WM_HOTKEY , OnHotKey)
	END_MSG_MAP()
public:
	CHideWindow(void);
	~CHideWindow(void);
	void InitHotKey();
	LRESULT OnCreate( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled );
	LRESULT OnHotKey( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled );
	LRESULT OnDestroy( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled );
	void DispLog();
private:
	boost::shared_ptr<CSHDispDialog> m_dispDialogPtr;
};


