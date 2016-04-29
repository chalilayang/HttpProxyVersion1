
#include "StdAfx.h"
#include "ShListControl.h"

map<HWND,ShListControl*> ShListControl::m_listMap;

#define  WM_SH_COPYTEXT (WM_USER + 6000)

ShListControl::ShListControl(HWND hwnd,int width,int heigh)
{
	m_hList	 = ::CreateWindow (WC_LISTVIEW, NULL, WS_CHILD | LVS_REPORT |WS_BORDER|LVS_SHOWSELALWAYS, 5, 
		5,width,heigh, hwnd, NULL, GetModuleHandle(NULL), NULL);
	m_wndProc = (WNDPROC)SetWindowLongPtr(m_hList,GWLP_WNDPROC,(LONG)&ShListControl::WindowProc);
	m_listMap.insert(std::make_pair(m_hList,this));
}

ShListControl::~ShListControl(void)
{
	m_listMap.erase(m_hList);
}

int	ShListControl::InsertColumn(int nCol,LPCTSTR szText,int width,int nFormat)
{
	LVCOLUMN column;
	column.mask = LVCF_TEXT|LVCF_FMT|LVCF_WIDTH;
	column.pszText = (LPTSTR)szText;
	column.fmt = nFormat;
	column.cx  = width;
	int index = (int) ::SendMessage(m_hList, LVM_INSERTCOLUMN, nCol, (LPARAM)&column);
	return index;
}

int	ShListControl::InsertItem(int nItem,LPCTSTR lpszItem)
{
	LVITEM item = {0};
	item.mask = LVIF_TEXT;
	item.iItem = nItem;
	item.iSubItem = 0;
	item.pszText = (LPTSTR)lpszItem;
	return (int) ::SendMessage(m_hList, LVM_INSERTITEM, 0, (LPARAM)&item);
}

BOOL ShListControl::SetItemText(int nItem,int nSubItem,LPCTSTR lpszText)
{
	LVITEM lvi;
	lvi.iSubItem = nSubItem;
	lvi.pszText = (LPTSTR) lpszText;
	return (BOOL) ::SendMessage(m_hList, LVM_SETITEMTEXT, nItem, (LPARAM)&lvi);
}

void ShListControl::SetItemData(int nItem,long data)
{
	LVITEM lvi = {0};
	lvi.mask = LVIF_PARAM;
	lvi.iItem = nItem;
	lvi.lParam = (LPARAM)data;
	::SendMessage(m_hList, LVM_SETITEM, 0, (LPARAM)&lvi);
}

long ShListControl::GetItemData(int nItem)const
{
	LVITEM lvi;
	memset(&lvi, 0, sizeof(LVITEM));
	lvi.iItem = nItem;
	lvi.mask = LVIF_PARAM;
	::SendMessage(m_hList, LVM_GETITEM, 0, (LPARAM)&lvi);
	return lvi.lParam;
}

DWORD ShListControl::GetExtendedStyle() const
{
	return (DWORD) ::SendMessage(m_hList, LVM_GETEXTENDEDLISTVIEWSTYLE, 0, 0);
}

DWORD ShListControl::SetExtendedStyle(DWORD dwNewStyle)
{
	return (DWORD) ::SendMessage(m_hList, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, (LPARAM) dwNewStyle);
}

int	ShListControl::GetItemCount()const
{
	return (int) ::SendMessage(m_hList, LVM_GETITEMCOUNT, 0, 0L);
}

int	ShListControl::GetColumnCount()const
{
	HWND hWnd = (HWND) ::SendMessage(m_hList, LVM_GETHEADER, 0, 0);
	if (hWnd == NULL)
		return 0;
	else
		return (int) ::SendMessage(hWnd, HDM_GETITEMCOUNT, 0, 0L); 
}

int ShListControl::GetSelectedItem()const
{
	return ListView_GetSelectionMark(m_hList);
}

void  ShListControl::SetSelectedItem(int nItem)
{
	int nSel = ListView_GetSelectionMark(m_hList);
	if(nSel >= 0)
		ListView_SetItemState(m_hList,nSel,~LVIS_SELECTED,LVIS_SELECTED);
	ListView_SetSelectionMark(m_hList,nItem);
	ListView_SetItemState(m_hList,nItem,LVIS_SELECTED,LVIS_SELECTED);
}

BOOL  ShListControl::EnsureVisible(int nItem,BOOL bPartialOK)
{
	return (BOOL) ::SendMessage(m_hList, LVM_ENSUREVISIBLE, nItem, MAKELPARAM(bPartialOK, 0));
}

HWND  ShListControl::GetListWnd()
{
	return m_hList;
}

CString  ShListControl::GetItemText(int nItem, int nSubItem) const
{
	LVITEM lvi;
	memset(&lvi, 0, sizeof(LVITEM));
	lvi.iSubItem = nSubItem;
	CString str;
	int nLen = 128;
	int nRes;
	do
	{
		nLen *= 2;
		lvi.cchTextMax = nLen;
		lvi.pszText = str.GetBufferSetLength(nLen);
		nRes  = (int)::SendMessage(m_hList, LVM_GETITEMTEXT, (WPARAM)nItem,
			(LPARAM)&lvi);
	} while (nRes >= nLen-1);
	return str;
}

BOOL ShListControl::CopyText(LPCTSTR szText)
 {
	 USES_CONVERSION;
 	if(!::OpenClipboard(m_hList))
 		return FALSE;
	char* pszText = T2A(szText);
 	HGLOBAL clipbuffer;
 	char * buffer;
 	EmptyClipboard();
 	clipbuffer = GlobalAlloc(GMEM_DDESHARE, strlen(pszText)+1);
 	buffer = (char *)GlobalLock(clipbuffer);
 	strcpy(buffer, pszText);
 	GlobalUnlock(clipbuffer);
 	SetClipboardData(CF_TEXT,clipbuffer);
 	CloseClipboard();
 	return TRUE;
}


LRESULT CALLBACK ShListControl::WindowProc(HWND hwnd,
										   UINT uMsg,
										   WPARAM wParam,
										   LPARAM lParam
										   )
{
	if(m_listMap.find(hwnd) == m_listMap.end())
		return 0;
	ShListControl* pThis = m_listMap.find(hwnd)->second;
	if(uMsg == WM_KEYDOWN && (wParam == 'c' || wParam == 'C') && GetAsyncKeyState(VK_CONTROL) & 0x8000)
	{
		PostMessage(hwnd,WM_SH_COPYTEXT,0,0);
		return 0;
	}
	else if(uMsg == WM_SH_COPYTEXT)
	{
		int nSel = ListView_GetSelectionMark(pThis->m_hList);
		if(nSel < 0)
			return 0;
		int count = pThis->GetColumnCount();
		CString strText;
		for (int i = 0; i < count; ++i)
		{
			CString strSubText = pThis->GetItemText(nSel,i);
			strText += strSubText;
			if(i != count-1)
				strText += _T("   ");
		}
		pThis->CopyText(strText);
		return 0;
	}
	else
	{
		return CallWindowProc(pThis->m_wndProc,hwnd,uMsg,wParam,lParam);
	}
}