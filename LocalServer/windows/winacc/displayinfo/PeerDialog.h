#pragma once
//#include "api\sh_p2p_system_define.h"
#include <commctrl.h>
#include "ShListControl.h"
#include <list>
#include "util.h"

class CSHPeerDialog
{
public:
	CSHPeerDialog(list<NewSHPeerInfo>* peerList);
	~CSHPeerDialog(void);
public:
	BOOL	Create();
	BOOL	Destroy();
	HWND	GetSafeHandle()const;
	BOOL	ShowWindow(int nShow);
	BOOL	IsVisable()const;
	void	Clear();
private:
	static INT_PTR CALLBACK DlgMessageProc( HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam );
	void	InitCtrl();
private:
	int		GetItemIndex(long data)const;
public:
	void    UpdatePeerInfo(const list<NewSHPeerInfo>* peerList,int dispItemID);
	void    SetDispItemID(int  imteID){m_dispId = imteID;} 
	void    SetDownState(bool val);
	void    SetCaptionName(LPCTSTR strName);
private:
	static HWND	m_hWnd;
	static HWND m_hList;
	ShListControl* m_peerMager;
	list<NewSHPeerInfo>*   m_peerList;
	bool				m_isDownCompleted;
	int					m_dispId;
};
