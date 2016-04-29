#include "StdAfx.h"
#include  "PeerDialog.h"
#include "..\Resource.h"
#include <CommCtrl.h>
#include "Util.h"
#include <list>
//#include "api\sh_p2p_system_define.h"


#define  WM_UPDATE_PEERINFO            WM_USER+10

HWND	CSHPeerDialog::m_hWnd = NULL;
HWND    CSHPeerDialog::m_hList = NULL;

CSHPeerDialog::CSHPeerDialog(list<NewSHPeerInfo>* peerList):m_isDownCompleted(false),m_dispId(0)
{
 	if(peerList == NULL )
 	{
		MessageBox(NULL,_T("peerList未初始化"),_T("错误"),0);
		return;
 	}
	m_peerList = peerList;
	Create();
}

CSHPeerDialog::~CSHPeerDialog(void)
{
	
}

BOOL CSHPeerDialog::Create()
{
	m_hWnd		 =  CreateDialog(GetModuleHandle(_T("winacc.exe")),MAKEINTRESOURCE(IDD_PEER_INFO),NULL,CSHPeerDialog::DlgMessageProc);

	RECT rect;
	GetWindowRect(m_hWnd,&rect);
	m_peerMager  = new ShListControl(m_hWnd,rect.right-rect.left,rect.bottom-rect.top);
	m_hList      = m_peerMager->GetListWnd();
	
	InitCtrl();
	DWORD dwHeight = GetSystemMetrics(SM_CYSCREEN);	 // 屏幕高度
	DWORD dwWidth = GetSystemMetrics(SM_CXSCREEN);	 // 屏幕宽度
	DWORD dwX,dwY;
	dwX = ( dwWidth - (rect.right + rect.left)*2.2 ) / 2;
	dwY = ( dwHeight - rect.bottom + rect.top ) / 2;
	SetWindowPos(m_hWnd,NULL,dwX,dwY,(rect.right-rect.left)*2.2,rect.bottom-rect.top,SWP_HIDEWINDOW);
	SendMessage(m_hWnd,WM_SIZE,0,0);
	return TRUE;
}

void	CSHPeerDialog::InitCtrl()
{	
	int index = 0;
	//初始化LIST 控件
	//整行选择但没有GRIDLINE
	m_peerMager->SetExtendedStyle(m_peerMager->GetExtendedStyle()&~LVS_EX_GRIDLINES|LVS_EX_FULLROWSELECT);
	m_peerMager->InsertColumn(index++,_T("序号"),40,LVCFMT_RIGHT);
	m_peerMager->InsertColumn(index++,_T("PeerID"),40,LVCFMT_RIGHT);
	m_peerMager->InsertColumn(index++,_T("KEY"),60);
	m_peerMager->InsertColumn(index++,_T("NAT类型"),60);
	m_peerMager->InsertColumn(index++,_T("可用IP"),100,LVCFMT_RIGHT);
	m_peerMager->InsertColumn(index++,_T("可用Port"),60,LVCFMT_RIGHT);
	m_peerMager->InsertColumn(index++,_T("打洞耗时"),60,LVCFMT_RIGHT);
	m_peerMager->InsertColumn(index++,_T("速度"),80,LVCFMT_RIGHT);
	m_peerMager->InsertColumn(index++,_T("平均速度"),80,LVCFMT_RIGHT);
	m_peerMager->InsertColumn(index++,_T("窗口"),40);
	m_peerMager->InsertColumn(index++,_T("平均RTTS"),60);
	m_peerMager->InsertColumn(index++,_T("lost率"),60);
	m_peerMager->InsertColumn(index++,_T("平均send"),70);
	m_peerMager->InsertColumn(index++,_T("平均receive"),80);
	m_peerMager->InsertColumn(index++,_T("平均timeout"),80);
	m_peerMager->InsertColumn(index++,_T("状态"),60);
}

BOOL CSHPeerDialog::Destroy()
{
	DestroyWindow(m_hWnd);
	delete m_peerMager;
	m_peerMager = NULL;
	return TRUE;
}

HWND CSHPeerDialog::GetSafeHandle()const
{
	return m_hWnd;
}


BOOL  CSHPeerDialog::ShowWindow(int nShow)
{
	BOOL result =  ::ShowWindow(m_hWnd,nShow);
	if(nShow == SW_SHOW)
		SetWindowPos(m_hWnd, HWND_TOPMOST,0,0,0,0,SWP_NOSIZE|SWP_NOMOVE);
	return result;
}

int CSHPeerDialog::GetItemIndex(long data)const
{
	int count = m_peerMager->GetItemCount();
	for (int i = 0; i < count; ++i)
	{
		if(m_peerMager->GetItemData(i) == data)
			return i;
	}
	return -1;
}

BOOL CSHPeerDialog::IsVisable()const
{
	return ::IsWindowVisible(m_hWnd);
}

void CSHPeerDialog::Clear()
{
	SendMessage(m_hList, LVM_DELETEALLITEMS, 0, 0L);
}


INT_PTR CALLBACK CSHPeerDialog::DlgMessageProc( HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam )
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_COMMAND:
		{
			if (LOWORD(wParam) == ID_CLEAR)
			{
				SendMessage(m_hList, LVM_DELETEALLITEMS, 0, 0L);
			}
			break;
		}
	case WM_SIZE:
		{
			HWND hList = m_hList;
			RECT rect;
			GetClientRect(hDlg,&rect);
			SetWindowPos(hList,NULL,rect.left,rect.top,rect.right-rect.left,rect.bottom-rect.top,SWP_SHOWWINDOW);
			HWND btn = GetDlgItem(hDlg,ID_CLEAR);
			if(btn)
				SetWindowPos(btn,NULL,rect.right-90,rect.bottom+10,0,0,SWP_NOSIZE|SWP_SHOWWINDOW);
			break;
		}
	case WM_INITDIALOG:
		{
// 			HICON hIcon = ::LoadIcon(GetModuleHandle(NULL),MAKEINTRESOURCE(IDI_ICON1));
// 			::SendMessage(hDlg, WM_SETICON, TRUE, (LPARAM)hIcon);
// 			::SendMessage(hDlg, WM_SETICON, FALSE, (LPARAM)hIcon);
// 			DestroyIcon(hIcon);
			return (INT_PTR)TRUE;
		}
	case  WM_GETMINMAXINFO:
		{
			MINMAXINFO* pinfo = (MINMAXINFO*)lParam;
			pinfo->ptMinTrackSize.x = 320;
			pinfo->ptMinTrackSize.y = 260;
			return (INT_PTR)TRUE;
		}
	case  WM_CLOSE:
		{
			::ShowWindow(hDlg,SW_HIDE);
		}
 		break;
	default:
		break;
	}
	return (INT_PTR)FALSE;
}


void    CSHPeerDialog::SetDownState(bool val)
{
	m_isDownCompleted = val;
}

void    CSHPeerDialog::UpdatePeerInfo(const list<NewSHPeerInfo>* peerList,int dispItemID)
{
	if(dispItemID != m_dispId)
		return;
	CString strText;
	int index = 0;
	list<NewSHPeerInfo>::const_iterator it;
	for (it = peerList->begin(); it != peerList->end(); it++,index++)
	{
		int subItem = 1;
		if(m_peerMager->GetItemCount() <= index)
		{
			strText.Format(_T("%d"),m_peerMager->GetItemCount()+1);
			m_peerMager->InsertItem(m_peerMager->GetItemCount(),strText);
		}
		//
		if(it->iscdn)
		{
			strText.Format(_T("%s"),_T("-:-:-"));
			//PeerID
			m_peerMager->SetItemText(index,subItem++,strText);
			//Key
			subItem++;
			//NAT类型
			m_peerMager->SetItemText(index,subItem++,strText);
		}
		else
		{
			//PeerID
			strText.Format(_T("%d"),it->peerId);
			m_peerMager->SetItemText(index,subItem++,strText);
			//Key
			strText.Format(_T("%d"),it->index_key);
			m_peerMager->SetItemText(index,subItem++,strText);
			//NAT类型
			strText.Format(_T("%s"),NatTypeDesc((SHNatType)it->nat).c_str());
			m_peerMager->SetItemText(index,subItem++,strText);
		}
		//可用IP
		m_peerMager->SetItemText(index,subItem++,GetIpStr(it->ip).c_str());
		//端口
		strText.Format(_T("%d"),ntohs(it->port));
		m_peerMager->SetItemText(index,subItem++,strText);

		//打洞耗时
		strText.Format(_T("%dms"),it->punch_time);
		m_peerMager->SetItemText(index,subItem++,strText);	
		
		CString strState;
		//状态
        if (it->status == SHPeerStatus_Fetching_Address)
        {
            strState = _T("取外网地址中");
        }
        else if (it->status == SHPeerStatus_Requsting)
        {
            strState = _T("正在请求文件");
        }
		else if(it->status == SHPeerStatus_Punching)
		{
			strState = _T("正在打洞");
		}
		else if(it->status == SHPeerStatus_PunchSucceed)
		{
			strState = _T("打洞成功");
		}
		else if(it->status == SHPeerStatus_PunchFailed)
		{
			strState = _T("打洞失败");
		}
		else if(it->status == SHPeerStatus_Connecting)
		{
			strState = _T("正在连接");
		}
		else if(it->status == SHPeerStatus_ConnectSucceed)
		{
			strState = _T("连接成功");
		}
		else if(it->status == SHPeerStatus_ConnectFailed)
		{
			strState = _T("连接失败");
		}
		else if(it->status == SHPeerStatus_Closed)
		{
			strState = _T("已关闭");
		}
		else if(it->status == SHPeerStatus_Pause)
		{
			strState = _T("暂停");
		}
		else if (it->status == SHPeerStatus_Unused)
		{
			strState = _T("未使用");
		}
		if(!m_isDownCompleted)
		{
			if(it->status == SHPeerStatus_ConnectSucceed)
			{
				//速度
				strText.Format(_T("%s/s"),GetSizeDesc(it->speed*1024).c_str());
				m_peerMager->SetItemText(index,subItem++,strText);
				//平均速度
				strText.Format(_T("%s/s"),GetSizeDesc(it->average_speed*1024).c_str());
				m_peerMager->SetItemText(index,subItem++,strText);
			}
			else
			{
				//速度
				strText.Format(_T("%s/s"),GetSizeDesc(0).c_str());
				m_peerMager->SetItemText(index,subItem++,strText);
				//平均速度
				strText.Format(_T("%s/s"),GetSizeDesc(0).c_str());
				m_peerMager->SetItemText(index,subItem++,strText);
			}
		}
		else
		{
			if(it->status == SHPeerStatus_ConnectSucceed)
			{
				strText.Format(_T("%s"),_T("-:-:-"));
				m_peerMager->SetItemText(index,subItem++,strText);
				//平均速度
				strText.Format(_T("%s/s"),GetSizeDesc(it->average_speed*1024).c_str());
				m_peerMager->SetItemText(index,subItem++,strText);
			}
			else
			{
				strText.Format(_T("%s"),_T("-:-:-"));
				m_peerMager->SetItemText(index,subItem++,strText);
				//平均速度
				strText.Format(_T("%s"),_T("-:-:-"));
				m_peerMager->SetItemText(index,subItem++,strText);
			}
		}
		
		//窗口大小
		strText.Format(_T("%d"),it->win_size);
		m_peerMager->SetItemText(index,subItem++,strText);	
		
		//平均RTTS
		strText.Format(_T("%d"),it->avg_rtt);
		m_peerMager->SetItemText(index,subItem++,strText);

		//lost率
		strText.Format(_T("%.2f%%"),it->lost_rate);
		m_peerMager->SetItemText(index,subItem++,strText);

		//平均send
		strText.Format(_T("%d"),it->avg_send_count);
		m_peerMager->SetItemText(index,subItem++,strText);

		//平均receive
		strText.Format(_T("%d"),it->avg_receive_count);
		m_peerMager->SetItemText(index,subItem++,strText);

		//平均timeout
		strText.Format(_T("%d"),it->avg_timeout_count);
		m_peerMager->SetItemText(index,subItem++,strText);

		//状态
		m_peerMager->SetItemText(index,subItem++,strState);	

	} 
}
void   CSHPeerDialog::SetCaptionName(LPCTSTR strName)
{
	SetWindowText(this->GetSafeHandle(),strName);
}