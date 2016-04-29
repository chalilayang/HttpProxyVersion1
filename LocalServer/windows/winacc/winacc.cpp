// winacc.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "winacc.h"
#include <string>
#include <time.h>
#include "displayinfo\HideWindow.h"
#include "../../include/sh_local_server_api.h"
#include "../../include/sh_p2p_system_define.h"
// #include "HttpClient.h"
// #include "ThreadPool.h"

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;								// current instance
TCHAR szTitle[MAX_LOADSTRING];					// The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];			// the main window class name

// Forward declarations of functions included in this code module:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);

void SHP2P_CALLBACK OnGetRegisterId(sh_int_32 register_id);

void OnGetVideoDataCallback(DWORD errorCode,DWORD statusCode,LPCSTR data,UINT len)
{

}


int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

 	// TODO: Place code here.
	MSG msg;
	HACCEL hAccelTable;

	// Initialize global strings
	/*LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_WINACC, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// Perform application initialization:
	if (!InitInstance (hInstance, nCmdShow))
	{
		return FALSE;
	}*/

	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_WINACC));
	//启动p2p系统
    SHP2PSystemParam system_param;
	SHP2pSystemNofity system_notify;
    //测试填充
    system_param.app_path = NULL;
    system_param.app_update_version = NULL;
    system_param.app_version = NULL;
    system_param.p2psys_version = NULL;
    system_param.install_time = NULL;
    system_param.machine_code = NULL;
    system_param.sohu_key = NULL;
    system_param.system_info = NULL;
	system_param.local_ip=GetLocalIp();
    //
    system_param.log_path = _T("D:\\test");
    //system_param.register_id = get_register_id();
	system_param.register_id = 0;
    system_param.allow_cache = true;
    system_param.cache_limit = 500;
    system_param.cache_path = _T("D:\\test");
	system_param.allow_log = 1;
	system_param.report=0;
	system_param.platform_type = PLATFORM_PC;

	system_notify.recv_video_data_notify_proc = receive_data;
	system_notify.get_video_duration_notify_proc = NULL;
	system_notify.recv_push_message_notify_proc=NULL;
	system_notify.register_succeed_notify_proc=register_notify;
	system_notify.video_error_notify_proc=NULL;
	system_notify.video_finish_notify_proc=NULL;
	
	//init_p2p_system(system_param,system_notify);
	
	//启动显示对话框(ctrl+alt+f9)
	CHideWindow hideWindow;
	hideWindow.Create(NULL,NULL,_T("搜狐影音-下载中心"));
	hideWindow.ShowWindow(SW_HIDE);
	hideWindow.UpdateWindow();
	hideWindow.InitHotKey();

#if 1
	//启动local server
	init_local_server(system_param);

	/*Sleep(3000);
	uninit_local_server();
	Sleep(3000);
	init_local_server(system_param);*/
	
	//http 请求
	/*Sleep(3000);
	SohuTool::CThreadPool::Instance().Init();	
	SohuTool::CHttpFactory::Instance().Init();

	ATL::CString strUrl;
	strUrl.Format(_T("http://127.0.0.1:8829?start=123456"));
	SohuTool::HttpClientPtr requestPtr = SohuTool::CreateWebRequest();
	requestPtr->SetRecvCallbck(boost::bind(OnGetVideoDataCallback,_1,_2,_3,_4));
	requestPtr->SetRecvBlockLen(256*1024);
	requestPtr->Request(SohuTool::EHttpRequest_Get,strUrl);*/
#endif 

	//完整流程
	//test_start_request_video_data(1301699, kSHVideoClarity_High,1,false, 0);
	//start_request_video_data(1301699, kSHVideoClarity_Supper,0,false);
	//notify_ad_duration(1301699,30);
	//跳转流程,第1段第1个block
    //start_request_video_data_time_ex(1301699, kSHVideoClarity_Supper,false,40);
	//跳转流程,第一段非第1个block
	//start_request_video_data_time_ex(1301699, kSHVideoClarity_Supper,false,200);
	// Main message loop:

	const int a[] = {1, 2, 3};
	//float b[a[2]]; //error

	const int buffersize = 100;
	float b[buffersize]; //ok


	
	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	
	//uninit_p2p_system();

	return (int) msg.wParam;
}

void SHP2P_CALLBACK OnGetRegisterId(sh_int_32 register_id)
{

}


//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
//  COMMENTS:
//
//    This function and its usage are only necessary if you want this code
//    to be compatible with Win32 systems prior to the 'RegisterClassEx'
//    function that was added to Windows 95. It is important to call this function
//    so that the application will get 'well formed' small icons associated
//    with it.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_WINACC));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= MAKEINTRESOURCE(IDC_WINACC);
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassEx(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   HWND hWnd;

   hInst = hInstance; // Store instance handle in our global variable

   hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   SetTimer(hWnd,2,1000,NULL);

   return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND	- process the application menu
//  WM_PAINT	- Paint the main window
//  WM_DESTROY	- post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;

	switch (message)
	{
	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		// Parse the menu selections:
		switch (wmId)
		{
		case IDM_ABOUT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		// TODO: Add any drawing code here...
		EndPaint(hWnd, &ps);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case WM_TIMER:
		//start_request_video_data(1301699, kSHVideoClarity_Supper,1,false);
		//跳转流程,第1段第1个block
		//start_request_video_data_time_ex(1301699, kSHVideoClarity_Supper,false,200);
		//跳转流程,第一段非第1个block
		//start_request_video_data_time_ex(1301699, kSHVideoClarity_Supper,false,200);
		//跳转流程,第2段非第1个block
		//start_request_video_data_time_ex(1301699, kSHVideoClarity_Supper,false,310);
		KillTimer(hWnd,2);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}