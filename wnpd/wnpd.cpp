// wnpd.cpp : ����Ӧ�ó������ڵ㡣
//

#include "stdafx.h"
#include "wnpd.h"
#include "trayicon.h"
#include "wnpd_server.h"

#define MAX_LOADSTRING 100
#define WM_TRAY_NOTIFICATION WM_USER+100

// ȫ�ֱ���:
HINSTANCE hInst;								// ��ǰʵ��
TCHAR szTitle[MAX_LOADSTRING];					// �������ı�
TCHAR szWindowClass[MAX_LOADSTRING];			// ����������
CTrayIcon* g_Tray;
HMENU hMenu;
BOOL bServerStarted = FALSE;
const wchar_t* szMutex = L"{28F4CC5C-88D7-4042-8E78-2D9D3B93AAA2}";

WnpdInfo wInfo;



// �˴���ģ���а����ĺ�����ǰ������:
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
 	// TODO: �ڴ˷��ô��롣
	MSG msg;
	HACCEL hAccelTable;

	// ��ʼ��ȫ���ַ���
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_WNPD, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// ִ��Ӧ�ó����ʼ��:
	if (!InitInstance (hInstance, nCmdShow))
	{
		return 1;
	}

	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_WNPD));

	// ����Ϣѭ��:
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
//  ����: MyRegisterClass()
//
//  Ŀ��: ע�ᴰ���ࡣ
//
//  ע��:
//
//    ����ϣ��
//    �˴�������ӵ� Windows 95 �еġ�RegisterClassEx��
//    ����֮ǰ�� Win32 ϵͳ����ʱ������Ҫ�˺��������÷������ô˺���ʮ����Ҫ��
//    ����Ӧ�ó���Ϳ��Ի�ù�����
//    ����ʽ��ȷ�ġ�Сͼ�ꡣ
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
//   ����: InitInstance(HINSTANCE, int)
//
//   Ŀ��: ����ʵ�����������������
//
//   ע��:
//
//        �ڴ˺����У�������ȫ�ֱ����б���ʵ�������
//        ��������ʾ�����򴰿ڡ�
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   HWND hWnd;

   hInst = hInstance; // ��ʵ������洢��ȫ�ֱ�����

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
	   MessageBox(NULL,L"nginx ���ò���ȷ���������޷�������",L"����",MB_OK|MB_ICONERROR);
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
   g_Tray->Create(LoadIcon(hInstance,MAKEINTRESOURCE(IDI_MAIN)),L"nginx �������������",hWnd,WM_TRAY_NOTIFICATION,IDI_MAIN);
   g_Tray->SetIcon(LoadIcon(hInstance,MAKEINTRESOURCE(IDI_MAIN)),L"nginx �������������");

   return TRUE;
}

HMENU CreateRBtnPopupMenu()
{
	HMENU hMenu = CreatePopupMenu();
	AppendMenu(hMenu,MF_STRING,IDM_START,L"����");
	AppendMenu(hMenu,MF_STRING,IDM_STOP,L"ֹͣ");
	AppendMenu(hMenu,MF_STRING,IDM_EXIT,L"�˳�");
	AppendMenu(hMenu,MF_STRING,IDM_ABOUT,L"����");
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
//  ����: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  Ŀ��: ���������ڵ���Ϣ��
//
//  WM_COMMAND	- ����Ӧ�ó���˵�
//  WM_PAINT	- ����������
//  WM_DESTROY	- �����˳���Ϣ������
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
		// �����˵�ѡ��:
		switch (wmId)
		{
		case IDM_ABOUT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;
		case IDM_EXIT:
			if(bServerStarted)
			{
				iRet = MessageBox(hWnd,L"�˳������������رպ�̨nginx��pnp-cgi����\n"
					L"ȷ���˳���",L"�˳�ȷ�ϣ�",MB_OKCANCEL|MB_ICONWARNING);
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

// �����ڡ������Ϣ�������
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
