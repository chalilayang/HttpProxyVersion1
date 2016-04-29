#pragma once
#include <commctrl.h>
#include <string>
#include <map>
#include <atlstr.h>
using namespace std;

class ShListControl
{
public:
	ShListControl(HWND hwnd,int width,int heigh);
	~ShListControl(void);

public:
	int		InsertColumn(int nCol,LPCTSTR szText,int width,int nFormat = LVCFMT_LEFT);
	int		InsertItem(int nItem,LPCTSTR lpszItem);
	BOOL	SetItemText(int nItem,int nSubItem,LPCTSTR lpszText);
	long	GetItemData(int nItem)const;
	DWORD	GetExtendedStyle() const;
	DWORD	SetExtendedStyle(DWORD dwNewStyle);
	int		GetItemCount()const;
	BOOL	EnsureVisible(int nItem,BOOL bPartialOK);
	HWND    GetListWnd();
	int     GetSelectedItem()const;
	int		GetColumnCount()const;
	CString GetItemText(int nItem, int nSubItem) const;
	void    SetSelectedItem(int nItem);
	void    SetItemData(int nItem,long data);
private:
	static LRESULT CALLBACK ShListControl::WindowProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
	BOOL CopyText(LPCTSTR szText);
private:
	HWND		m_hList;
	WNDPROC		m_wndProc;
	static map<HWND,ShListControl*> m_listMap;
};
