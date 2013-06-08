// wnpd.cpp : 定义应用程序的入口点。
//

#include "stdafx.h"
#include "wnpd.h"
#include "trayicon.h"
#include "wnpd_server.h"

#define MAX_LOADSTRING 100
#define WM_TRAY_NOTIFICATION WM_USER+100

// 全局变量:
HINSTANCE hInst;								// 当前实例
TCHAR szTitle[MAX_LOADSTRING];					// 标题栏文本
TCHAR szWindowClass[MAX_LOADSTRING];			// 主窗口类名
CTrayIcon* g_Tray;
HMENU hMenu;
BOOL bServerStarted = FALSE;
const wchar_t* szMutex = L"{28F4CC5C-88D7-4042-8E78-2D9D3B93AAA2}";

WnpdInfo wInfo;



// 此代码模块中包含的函数的前向声明:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	TCHAR szExePath[MAX_PATH];
	GetExePath(szExePath);
	SetCurrentDirectory(szExePath);

	HANDLE hObject;
	hObject = CreateMutex(NULL,FALSE,szMutex);

	if(ERROR_ALREADY_EXISTS == GetLastError())
	{
		CloseHandle(hObject);
		return 1;
	}
 	// TODO: 在此放置代码。
	MSG msg;
	HACCEL hAccelTable;

	// 初始化全局字符串
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_WNPD, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// 执行应用程序初始化:
	if (!InitInstance (hInstance, nCmdShow))
	{
		return 1;
	}

	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_WNPD));

	// 主消息循环:
	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return (int) msg.wParam;
}



//
//  函数: MyRegisterClass()
//
//  目的: 注册窗口类。
//
//  注释:
//
//    仅当希望
//    此代码与添加到 Windows 95 中的“RegisterClassEx”
//    函数之前的 Win32 系统兼容时，才需要此函数及其用法。调用此函数十分重要，
//    这样应用程序就可以获得关联的
//    “格式正确的”小图标。
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
	wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MAIN));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= NULL;
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_MAIN));

	return RegisterClassEx(&wcex);
}

//
//   函数: InitInstance(HINSTANCE, int)
//
//   目的: 保存实例句柄并创建主窗口
//
//   注释:
//
//        在此函数中，我们在全局变量中保存实例句柄并
//        创建和显示主程序窗口。
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   HWND hWnd;

   hInst = hInstance; // 将实例句柄存储在全局变量中

   hWnd = CreateWindowEx(WS_EX_NOACTIVATE,szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      0, 0, 0, 0, NULL, NULL, hInstance, NULL);

   if (!hWnd)
   {
      return FALSE;
   }

   GetIniInfo(wInfo);
   if(!VeryfyIniConfig(wInfo))
   {
	   return FALSE;
   }
   if(!CheckConf(wInfo))
   {
	   MessageBox(NULL,L"nginx 配置不正确，服务器无法启动！",L"错误！",MB_OK|MB_ICONERROR);
	   return FALSE;
   }
   else
   {
	   KillNginx(wInfo);
	   StartNginxPhpService(wInfo);
	   bServerStarted = TRUE;
   }

   ShowWindow(hWnd, SW_HIDE);
   UpdateWindow(hWnd);

   g_Tray = new CTrayIcon;
   g_Tray->Create(LoadIcon(hInstance,MAKEINTRESOURCE(IDI_MAIN)),L"nginx 服务器管理程序",hWnd,WM_TRAY_NOTIFICATION,IDI_MAIN);
   g_Tray->SetIcon(LoadIcon(hInstance,MAKEINTRESOURCE(IDI_MAIN)),L"nginx 服务器管理程序");

   return TRUE;
}

HMENU CreateRBtnPopupMenu()
{
	HMENU hMenu = CreatePopupMenu();
	AppendMenu(hMenu,MF_STRING,IDM_START,L"启动");
	AppendMenu(hMenu,MF_STRING,IDM_STOP,L"停止");
	AppendMenu(hMenu,MF_STRING,IDM_EXIT,L"退出");
	AppendMenu(hMenu,MF_STRING,IDM_ABOUT,L"关于");
	return hMenu;
}

void PopUpRBtnMenu( POINT &point, HWND hWnd )
{
	if(bServerStarted)
	{
		EnableMenuItem(hMenu,IDM_START,MF_GRAYED|MF_BYCOMMAND);
		EnableMenuItem(hMenu,IDM_STOP,MF_BYCOMMAND|MF_ENABLED);
	}
	else
	{
		EnableMenuItem(hMenu,IDM_START,MF_ENABLED|MF_BYCOMMAND);
		EnableMenuItem(hMenu,IDM_STOP,MF_BYCOMMAND|MF_GRAYED);
	}

	SetForegroundWindow(hWnd);
	TrackPopupMenu(hMenu,TPM_RIGHTALIGN,point.x,point.y,0,hWnd,NULL);
}

//
//  函数: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  目的: 处理主窗口的消息。
//
//  WM_COMMAND	- 处理应用程序菜单
//  WM_PAINT	- 绘制主窗口
//  WM_DESTROY	- 发送退出消息并返回
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	POINT point;
	int iRet;

	switch (message)
	{
	case WM_CREATE:		
		hMenu = CreateRBtnPopupMenu();
		return 0;

	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		// 分析菜单选择:
		switch (wmId)
		{
		case IDM_ABOUT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;
		case IDM_EXIT:
			if(bServerStarted)
			{
				iRet = MessageBox(hWnd,L"退出管理器，将关闭后台nginx及pnp-cgi服务！\n"
					L"确定退出吗？",L"退出确认：",MB_OKCANCEL|MB_ICONWARNING);
				if(IDOK == iRet)
				{
					bServerStarted = FALSE;
				}
				else
				{
					bServerStarted = TRUE;
				}
			}
			if(!bServerStarted)
			{
				g_Tray->Delete();
				delete g_Tray;
				DestroyWindow(hWnd);
			}
			break;
		case IDM_START:
			StartNginxPhpService(wInfo);
			bServerStarted = TRUE;
			break;
		case IDM_STOP:
			StopPhp(wInfo);
			StopNginx(wInfo);
			bServerStarted = FALSE;
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;
	case WM_DESTROY:
		StopPhp(wInfo);
		StopNginx(wInfo);
		PostQuitMessage(0);
		break;
	case WM_TRAY_NOTIFICATION:
		switch(lParam)			
		{
		case WM_RBUTTONDOWN:
			GetCursorPos(&point);
			PopUpRBtnMenu(point, hWnd);
			break;
		default:
			break;
		}
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

// “关于”框的消息处理程序。
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	//HICON hIcon;
	//HDC hDc;
	switch (message)
	{
	case WM_INITDIALOG:
		/*hIcon = LoadIcon(NULL,MAKEINTRESOURCE(IDI_MAIN));
		hDc = GetDC(hDlg);
		DrawIcon(hDc,10,10,hIcon);
		ReleaseDC(hDlg,hDc);*/
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
