#include <stdafx.h>
#include "trayicon.h"

BOOL CTrayIcon::Create( HICON hIcon,LPCTSTR lpTip,HWND pNotifyWnd,UINT uMsg,UINT uID )
{
	m_nidata.hIcon = hIcon;
	lstrcpyn(m_nidata.szTip,lpTip,sizeof(m_nidata.szTip));
	m_nidata.hWnd = pNotifyWnd;
	m_nidata.uID = uID;
	m_nidata.uCallbackMessage = uMsg;
	m_nidata.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
	return Shell_NotifyIcon(NIM_ADD, &m_nidata);
}

CTrayIcon::CTrayIcon()
{
	m_nidata.cbSize = sizeof(NOTIFYICONDATA);
	m_nidata.hIcon = NULL;
	m_nidata.hWnd = NULL;
	m_nidata.szTip[0] = 0;
	m_nidata.uCallbackMessage = NULL;
	m_nidata.uFlags = 0;
	m_nidata.uID = 0;
}

BOOL CTrayIcon::SetIcon( HICON hIcon,LPCTSTR lpTip )
{
	DWORD dwMessage;
	if (m_nidata.hIcon)
		dwMessage = NIM_MODIFY;
	else
		dwMessage = NIM_ADD;

	if (hIcon)
		m_nidata.hIcon = hIcon;
	if (lpTip)
		lstrcpyn(m_nidata.szTip, lpTip, sizeof(m_nidata.szTip));

	return Shell_NotifyIcon(dwMessage, &m_nidata);
}


BOOL CTrayIcon::Delete()
{
	if (m_nidata.hIcon)
	{
		BOOL bRet = Shell_NotifyIcon(NIM_DELETE, &m_nidata);
		m_nidata.hIcon = NULL;
		return bRet;
	}

	return TRUE;
}
