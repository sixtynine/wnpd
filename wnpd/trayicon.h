#pragma once


#include <ShellAPI.h>
//CTrayIcon ���ඨ��


class CTrayIcon
{
public:
	CTrayIcon();
	virtual ~CTrayIcon(){Delete();}

	BOOL Create(HICON hIcon,LPCTSTR lpTip,HWND pNotifyWnd,UINT uMsg,UINT uID);
	BOOL SetIcon(HICON hIcon,LPCTSTR lpTip);
	BOOL Delete();
//	virtual LRESULT OnNotifyIcon(WPARAM wID,LPARAM lEvent);
	HWND GetHwnd() const {return m_nidata.hWnd;}

	NOTIFYICONDATA m_nidata;
};