#pragma once
#include <commctrl.h>
#include "ShListControl.h"
#include <map>
#include "PeerDialog.h"

class CSHDispDialog
{
public:
	CSHDispDialog(void);
	~CSHDispDialog(void);
public:
	BOOL	Create();
	BOOL	Destroy();
	HWND	GetSafeHandle()const;
	BOOL	ShowWindow(int nShow);
	static void	DispInfo(const NewSHDispInfo& info);
	static BOOL	IsVisable();
	static void	Clear();
private:
	static INT_PTR CALLBACK DlgMessageProc( HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam );
	void	InitCtrl();
	static void    DisStart(const NewSHDispInfo& info);
// 	int		GetItemCount()const;
// 	BOOL	EnsureVisible(int nItem,BOOL bPartialOK);
private:
	static int	GetItemIndex(long data);
private:
	static HWND	m_hWnd;
	static HWND m_hList;
	static ShListControl* m_peerMager;
	static map<int,list<NewSHPeerInfo> > m_peerInfoHandle;
	static list<NewSHPeerInfo> peer_list;
	static int m_usablePeer;
	static int m_allUsablePeer;
public:
	static CSHPeerDialog*    m_peerDlg;
	
};
