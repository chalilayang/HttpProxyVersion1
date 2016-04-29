#include "StdAfx.h"
#include "DispDialog.h"
#include <CommCtrl.h>
//#include "api\sh_p2p_system_define.h"
#include <tchar.h>
#include <time.h>
#include "..\Resource.h"


HWND	CSHDispDialog::m_hWnd = NULL;
HWND    CSHDispDialog::m_hList = NULL;
ShListControl* CSHDispDialog::m_peerMager=NULL;
list<NewSHPeerInfo> CSHDispDialog::peer_list;
int CSHDispDialog::m_usablePeer=0;
int CSHDispDialog::m_allUsablePeer=0;
map<int,list<NewSHPeerInfo> >  CSHDispDialog::m_peerInfoHandle;
CSHPeerDialog*              CSHDispDialog::m_peerDlg = NULL ;
CSHDispDialog::CSHDispDialog(void)
{
	//TRACE_OBJCREATE(_T("CSHDispDialog"));
	peer_list.clear();
}

CSHDispDialog::~CSHDispDialog(void)
{
	//TRACE_OBJDESTROY(_T("CSHDispDialog"));
	m_hWnd = NULL;
	
}

BOOL CSHDispDialog::Create()
{
	
	m_hWnd = CreateDialog(GetModuleHandle(_T("winacc.exe")),MAKEINTRESOURCE(IDD_DISP_DIALOG),NULL,CSHDispDialog::DlgMessageProc);
	
	if(!m_hWnd)
	{
		int error=GetLastError();
	}
	RECT rect;
	GetWindowRect(m_hWnd,&rect);

	m_peerMager  = new ShListControl(m_hWnd,rect.right-rect.left-10,rect.bottom-rect.top-15);
	m_hList      = m_peerMager->GetListWnd();
	
	//添加HEADER信息
	InitCtrl();
	DWORD dwHeight = GetSystemMetrics(SM_CYSCREEN);	 // 屏幕高度
	DWORD dwWidth = GetSystemMetrics(SM_CXSCREEN);	 // 屏幕宽度
	DWORD dwX,dwY;
	dwX = ( dwWidth - (rect.right - rect.left)*2.8 ) / 2;
	dwY = ( dwHeight - rect.bottom + rect.top ) / 2;
	SetWindowPos(m_hWnd,NULL,dwX,dwY,(rect.right-rect.left)*2.8,rect.bottom-rect.top,SWP_HIDEWINDOW);
	SendMessage(m_hWnd,WM_SIZE,0,0);
	SetTimer(m_hWnd,1,1000,NULL);
	return TRUE;
}

void	CSHDispDialog::InitCtrl()
{	
	int index = 0;
	//初始化LIST 控件
	//整行选择但没有GRIDLINE
	m_peerMager->SetExtendedStyle(m_peerMager->GetExtendedStyle()&~LVS_EX_GRIDLINES|LVS_EX_FULLROWSELECT);
	m_peerMager->InsertColumn(index++,_T("序号"),40,LVCFMT_RIGHT);
	m_peerMager->InsertColumn(index++,_T("文件名"),120);
	m_peerMager->InsertColumn(index++,_T("段号"),40,LVCFMT_RIGHT);
	m_peerMager->InsertColumn(index++,_T("大小"),60,LVCFMT_RIGHT);
	m_peerMager->InsertColumn(index++,_T("时长"),40,LVCFMT_RIGHT);
	m_peerMager->InsertColumn(index++,_T("比特率"),60,LVCFMT_RIGHT);
	m_peerMager->InsertColumn(index++,_T("状态机"),90,LVCFMT_RIGHT);
	m_peerMager->InsertColumn(index++,_T("速度"),90,LVCFMT_RIGHT);
	m_peerMager->InsertColumn(index++,_T("平均速度"),90,LVCFMT_RIGHT);
	m_peerMager->InsertColumn(index++,_T("百分比"),60,LVCFMT_RIGHT);
	m_peerMager->InsertColumn(index++,_T("CDN数目"),40);
	m_peerMager->InsertColumn(index++,_T("max连接数"),80);
	m_peerMager->InsertColumn(index++,_T("peer数量"),65);
	m_peerMager->InsertColumn(index++,_T("开始时间"),70);
	m_peerMager->InsertColumn(index++,_T("结束时间"),70);
	m_peerMager->InsertColumn(index++,_T("状态"),60);
	m_peerMager->InsertColumn(index++,_T("耗时(S)"),60,LVCFMT_RIGHT);
	m_peerMager->InsertColumn(index++,_T("上报数据"),40,LVCFMT_RIGHT);
	m_peerMager->InsertColumn(index++,_T("节约比"),60,LVCFMT_RIGHT);
	m_peerMager->InsertColumn(index++,_T("类型"),40);
	m_peerMager->InsertColumn(index++,_T("起点"),40);
}

BOOL CSHDispDialog::Destroy()
{
	DestroyWindow(m_hWnd);
	delete m_peerMager;
	delete m_peerDlg;
	m_peerDlg = NULL;
	m_peerMager = NULL;
	m_peerInfoHandle.clear();
	m_hWnd = NULL;
	return TRUE;
}

HWND CSHDispDialog::GetSafeHandle()const
{
	return m_hWnd;
}


BOOL  CSHDispDialog::ShowWindow(int nShow)
{
	BOOL result =  ::ShowWindow(m_hWnd,nShow);
	if(nShow == SW_SHOW)
		SetWindowPos(m_hWnd, HWND_TOPMOST,0,0,0,0,SWP_NOSIZE|SWP_NOMOVE);
	return result;
}


void CSHDispDialog::DispInfo(const NewSHDispInfo& info)
{
	//计算可用Peer数量
	m_usablePeer = 0;
	m_allUsablePeer = 0;
	peer_list.clear();
	for(int i=0;i<info.peer_list_len;i++)
	{
		if(!info.peer_list[i].iscdn)
		{
			m_allUsablePeer++;
			if(info.peer_list[i].status == SHPeerStatus_ConnectSucceed)//连接成功
			{
				m_usablePeer++;
			}
		}
		peer_list.push_back(info.peer_list[i]);
	}

	CString strText;
	if(info.oper == 0)		//开始下载
	{
		int item = GetItemIndex(info.ID);
		if(item == -1)
		{
			DisStart(info);
		}
	}
	else if(info.oper == 1)	//更新显示速度
	{

		int item = GetItemIndex(info.ID);
		if(item == -1)
		{
			DisStart(info);
		}

		map<int, list<NewSHPeerInfo> >::iterator iter;
		m_peerInfoHandle[(int)info.ID] = peer_list;

		int subitem=5;
		
		strText.Format(_T("%d"),info.byterate);
		m_peerMager->SetItemText(item,subitem++,strText);

		switch(info.state_code)
		{
		case 1:
			m_peerMager->SetItemText(item,subitem++,_T("P2P"));
			break;
		case 2:
			m_peerMager->SetItemText(item,subitem++,_T("HTTP"));
			break;
		case 3:
			m_peerMager->SetItemText(item,subitem++,_T("HTTP|P2P"));
			break;
		case 4:
			m_peerMager->SetItemText(item,subitem++,_T("START"));
			break;
		case 5:
			m_peerMager->SetItemText(item,subitem++,_T("START|P2P"));
			break;
		case 6:
			m_peerMager->SetItemText(item,subitem++,_T("START|HTTP"));
			break;
		default:
			break;
		}

		strText.Format(_T("%s/s"),GetSizeDesc(info.speed*1024).c_str());
		m_peerMager->SetItemText(item,subitem++,strText);
		//
		strText.Format(_T("%s/s"),GetSizeDesc(info.average_speed*1024).c_str());
		m_peerMager->SetItemText(item,subitem++,strText);
		//
		strText.Format(_T("%.2f%%"),info.percent);
		m_peerMager->SetItemText(item,subitem++,strText);
		//
		strText.Format(_T("%d"),info.cdn_num);
		m_peerMager->SetItemText(item,subitem++,strText);

		strText.Format(_T("%d"),info.max_conn);
		m_peerMager->SetItemText(item,subitem++,strText);
		//
		strText.Format(_T("%d/%d"),m_usablePeer,m_allUsablePeer);
		m_peerMager->SetItemText(item,subitem++,strText);

		if(m_peerDlg != NULL && m_peerDlg->IsVisable())
		{
			m_peerDlg->SetDownState(false);
			m_peerDlg->UpdatePeerInfo(&peer_list,(int)info.ID);
		}
	}
	else					//下载完毕或出错
	{
		int item = GetItemIndex(info.ID);
		if(item == -1)
		{
			DisStart(info);
		}

		int subitem=7;

		//完成也要刷新下速度
		map<int, list<NewSHPeerInfo> >::iterator iter;
		m_peerInfoHandle[(int)info.ID] = peer_list;

		//ATLTRACE(_T("Stop item = %d,param = %d\r\n"),item,info.ID);
		m_peerMager->SetItemText(item,subitem++,_T("-:-:-"));
		//
		strText.Format(_T("%s/s"),GetSizeDesc(info.average_speed*1024).c_str());
		m_peerMager->SetItemText(item,subitem++,strText);

		if(info.state == 0)
		{
			m_peerMager->SetItemText(item,subitem++,_T("100%"));
		}
		else if(info.state == 1)
		{
			strText.Format(_T("%.2f%%"),info.percent);
			m_peerMager->SetItemText(item,subitem++,strText);	
		}
		else
		{
			strText.Format(_T("%.2f%%"),info.percent);
			m_peerMager->SetItemText(item,subitem++,strText);
		}

		strText.Format(_T("%d"),info.cdn_num);
		m_peerMager->SetItemText(item,subitem++,strText);
		
		subitem++;

		strText.Format(_T("%d/%d"),m_usablePeer,m_allUsablePeer);
		m_peerMager->SetItemText(item,subitem++,strText);

		subitem += 1;

		time_t ti(info.end_time);
		tm temptm = *localtime(&ti);
		SYSTEMTIME st = {1900 + temptm.tm_year, 
			1 + temptm.tm_mon, 
			temptm.tm_wday, 
			temptm.tm_mday, 
			temptm.tm_hour, 
			temptm.tm_min, 
			temptm.tm_sec, 
			0};
		strText.Format(_T("%02d-%02d %02d:%02d:%02d"),st.wMonth,st.wDay,st.wHour,st.wMinute,st.wSecond);
		m_peerMager->SetItemText(item,subitem++,strText);

		if(info.state == 0)
		{
			m_peerMager->SetItemText(item,subitem++,_T("下载完成"));
		}
		else if(info.state == 1)
		{
			m_peerMager->SetItemText(item,subitem++,_T("结束会话"));
		}
		else
		{
			strText.Format(_T("下载出错 errorCode %d"),info.state);
			m_peerMager->SetItemText(item,subitem++,strText);
		}
		
		
		//耗时
		strText.Format(_T("%.2f"),(float)info.timespace/1000);
		m_peerMager->SetItemText(item,subitem++,strText);
		//上报数据

		strText.Format(_T("%d/%s"),info.report_len,GetSizeDesc(info.report_len).c_str());
		m_peerMager->SetItemText(item,subitem++,strText);
		//
		strText.Format(_T("%.2f%%"),info.p2p_percent);
		m_peerMager->SetItemText(item,subitem++,strText);
		//
		if(m_peerDlg != NULL && m_peerDlg->IsVisable() )
		{
			m_peerDlg->SetDownState(true);
			m_peerDlg->UpdatePeerInfo(&peer_list,(int)info.ID);
		}
	}
}

int CSHDispDialog::GetItemIndex(long data)
{
	int count = m_peerMager->GetItemCount();
	for (int i = 0; i < count; ++i)
	{
			if(m_peerMager->GetItemData(i) == data)
			return i;
	}
	return -1;
}

BOOL CSHDispDialog::IsVisable()
{
	return ::IsWindowVisible(m_hWnd);
}

void CSHDispDialog::Clear()
{
	m_peerInfoHandle.clear();
	SendMessage(m_hList, LVM_DELETEALLITEMS, 0, 0L);
}


INT_PTR CALLBACK CSHDispDialog::DlgMessageProc( HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam )
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_COMMAND:
		{
			if (LOWORD(wParam) == ID_CLEAR)
			{
				m_peerInfoHandle.clear();
				SendMessage(m_hList, LVM_DELETEALLITEMS, 0, 0L);
			}
			break;
		}
	case WM_SIZE:
		{
			HWND hList = m_hList;
			RECT rect;
			GetClientRect(hDlg,&rect);
			rect.top	+= 5;
			rect.bottom -= 40;
			rect.left	+= 5;
			rect.right	-= 5;
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
	case WM_NOTIFY :
		{
			LPNMHDR pHdr = (LPNMHDR) lParam;
			if(pHdr!=NULL && pHdr->code == NM_DBLCLK)
			{
 				LPNMITEMACTIVATE lpNMItemActivate = (LPNMITEMACTIVATE)pHdr;
 				int nItem = -1;
 				if(lpNMItemActivate != NULL)
 				{
 					nItem = lpNMItemActivate->iItem;
 				}
				if( nItem!= -1 )
				{
					LVITEM lvi;
					memset(&lvi, 0, sizeof(LVITEM));
					lvi.iItem = nItem;
					lvi.mask = LVIF_PARAM;
					lvi.iSubItem = 0;
					::SendMessage(m_hList, LVM_GETITEM, 0, (LPARAM)&lvi); 
					
 					map<int, list<NewSHPeerInfo> >::iterator iter;
					iter = m_peerInfoHandle.find((int)lvi.lParam);


					if( m_peerDlg == NULL )
					{
						m_peerDlg = new CSHPeerDialog(&iter->second);
					}
					TCHAR pstr[512] = {0};
					LVITEM Celltext;
					memset(&Celltext, 0, sizeof(LVITEM));
					Celltext.mask = LVIF_TEXT;
					Celltext.iSubItem = 11;
					Celltext.pszText	= pstr;
					Celltext.cchTextMax	= sizeof(pstr)/sizeof(TCHAR);
					int i = ::SendMessage(m_hList, LVM_GETITEMTEXT, nItem, (LPARAM)&Celltext);
					if(_tcscmp(Celltext.pszText,_T("正在下载")) == 0)
					{
						m_peerDlg->SetDownState(false);//
					}
					else
					{
						m_peerDlg->SetDownState(true);
					}
					CString peerDlgText ;
					CString fname;
					Celltext.iSubItem = 1;
					::SendMessage(m_hList, LVM_GETITEMTEXT, nItem, (LPARAM)&Celltext);
					fname = Celltext.pszText;
					Celltext.iSubItem = 2;
					::SendMessage(m_hList, LVM_GETITEMTEXT, nItem, (LPARAM)&Celltext);
					peerDlgText.Format(_T("文件:%s 段号 :%s"),fname,Celltext.pszText);
					m_peerDlg->SetCaptionName(peerDlgText);
					m_peerDlg->SetDispItemID(iter->first);
					m_peerDlg->Clear();
					m_peerDlg->ShowWindow(SW_SHOW);
					m_peerDlg->UpdatePeerInfo(&iter->second,iter->first);
				}
			}
			break;
		}
	case WM_TIMER:
		{
			NewSHDispInfo info;
// 			while(fetch_display_info(&info))
// 			{
// 				DispInfo(info);
// 			}
			break;
		}
	
	default:
		break;
	}
	return (INT_PTR)FALSE;
}

void CSHDispDialog::DisStart(const NewSHDispInfo& info)
{
	CString strText;
	if(m_peerMager->GetItemCount()>=100)
	{
		Clear();
	}
	strText.Format(_T("%d"),m_peerMager->GetItemCount()+1);
	int item	= m_peerMager->InsertItem(m_peerMager->GetItemCount(),strText);
	m_peerMager->SetSelectedItem(item);
	int subItem	= 1;
	CString filename(info.file_name);
	m_peerMager->SetItemText(item,subItem++,filename);
	//
	strText.Format(_T("%d"),info.num);
	m_peerMager->SetItemText(item,subItem++,strText);
	
	m_peerMager->SetItemText(item,subItem++,GetSizeDesc(info.size).c_str());
	
	strText.Format(_T("%ds"),info.duration);
	m_peerMager->SetItemText(item,subItem++,strText);

	subItem += 5;
	//
	strText.Format(_T("%d"),info.cdn_num);
	m_peerMager->SetItemText(item,subItem++,strText);
	
	subItem++;

	strText.Format(_T("%d/%d"),m_usablePeer,m_allUsablePeer);
	m_peerMager->SetItemText(item,subItem++,strText);

	time_t ti(info.start_time);
	tm temptm = *localtime(&ti);
	SYSTEMTIME st = {1900 + temptm.tm_year, 
		1 + temptm.tm_mon, 
		temptm.tm_wday, 
		temptm.tm_mday, 
		temptm.tm_hour, 
		temptm.tm_min, 
		temptm.tm_sec, 
		0};
	strText.Format(_T("%02d-%02d %02d:%02d:%02d"),st.wMonth,st.wDay,st.wHour,st.wMinute,st.wSecond);
	m_peerMager->SetItemText(item,subItem++,strText);
	
	strText.Format(_T("-:-:-"));
	m_peerMager->SetItemText(item,subItem++,strText);
	
	m_peerMager->SetItemText(item,subItem++,_T("正在下载"));
	m_peerMager->SetItemData(item,info.ID);
	
	subItem += 3;

	if(info.type == 0)
	{
		m_peerMager->SetItemText(item,subItem++,_T("完整"));
	}
	else if(info.type == 1)
	{
		m_peerMager->SetItemText(item,subItem++,_T("拖拽"));
	}
	
	strText.Format(_T("%d"),info.play_start);
	m_peerMager->SetItemText(item,subItem++,strText);
	
	m_peerInfoHandle.insert(std::make_pair(info.ID,peer_list));

	if(IsVisable())
	{
		m_peerMager->EnsureVisible(m_peerMager->GetItemCount()-1,FALSE);
	}
	if(m_peerDlg != NULL && m_peerDlg->IsVisable() )
	{
		CString peerDlgText = _T("");
		peerDlgText.Format(_T("文件:%s 段号:%d"),info.file_name,info.num);
		m_peerDlg->SetCaptionName(peerDlgText.GetBuffer());
		m_peerDlg->Clear();
		m_peerDlg->SetDownState(false);
		m_peerDlg->SetDispItemID((int)info.ID);
		m_peerDlg->UpdatePeerInfo(&peer_list,(int)info.ID);
	}
}


