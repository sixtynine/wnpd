// wnpd.cpp : 定义应用程序的入口点。
//

#include "stdafx.h"
#include "wnpd_server.h"

HANDLE hStdOutRead,hStdOutWrite;


DWORD FindProcess( wchar_t *strProcessName )
{
	DWORD aProcesses[1024], cbNeeded, cbMNeeded;
	HMODULE hMods[1024];
	HANDLE hProcess;
	wchar_t szProcessName[MAX_PATH];

	if ( !EnumProcesses( aProcesses, sizeof(aProcesses), &cbNeeded ) )  return 0;
	for(int i=0; i< (int) (cbNeeded / sizeof(DWORD)); i++)
	{
		hProcess = OpenProcess( PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, aProcesses[i]);
		EnumProcessModules(hProcess, hMods, sizeof(hMods), &cbMNeeded);
		GetModuleFileNameEx( hProcess, hMods[0], szProcessName,sizeof(szProcessName));

		if(wcsstr(szProcessName, strProcessName))
		{
			return(aProcesses[i]);
		}
	}


	return 0;
}

BOOL KillProcess( const wchar_t* szProcessName )
{
	BOOL bRet;
	DWORD hProcessiD = FindProcess((wchar_t*)szProcessName);
	if(hProcessiD == 0)
		return FALSE;

	HANDLE hTargetProcess = OpenProcess(PROCESS_TERMINATE, FALSE, hProcessiD);

	if(hTargetProcess == NULL)
	{
		return FALSE;
	}

	bRet = TerminateProcess(hTargetProcess, 0);
	WaitForSingleObject(hTargetProcess,500);
	return bRet;
}

//转换UTF8字符串为宽字节
wstring UTF8ToUnicode(const string& str)
{
	int  unicodeLen = ::MultiByteToWideChar( CP_UTF8,0,str.c_str(),-1,NULL,0);  
	wchar_t *  pUnicode;  
	pUnicode = new  wchar_t[unicodeLen];  
	memset(pUnicode,0,(unicodeLen)*sizeof(wchar_t));  
	::MultiByteToWideChar( CP_UTF8,0,str.c_str(),-1,(LPWSTR)pUnicode, unicodeLen);  
	wstring  rt;  
	rt = ( wchar_t* )pUnicode;
	delete [] pUnicode; 
	return  rt;  
}

string UnicodeToUTF8(const wstring& wStr)
{
	char* pElementText;
	int iTextLen;
	// wide char to multi char
	iTextLen = WideCharToMultiByte( CP_UTF8, 0,wStr.c_str(),-1,NULL, 0,NULL,NULL);

	pElementText = new char[iTextLen];
	memset( ( void* )pElementText, 0, sizeof( char ) * ( iTextLen ) );
	::WideCharToMultiByte( CP_UTF8, 0, wStr.c_str(), -1, pElementText,iTextLen,NULL,NULL);

	string strText;
	strText = pElementText;
	delete[] pElementText;
	return strText;
}

BOOL GetExePath( wchar_t *szPathBuf ,int len /*= MAX_PATH*/)
{
	//获得url.exe绝对路径
	DWORD dwRet = GetModuleFileName(NULL, szPathBuf, len);	
	wchar_t *p = wcsrchr(szPathBuf,'\\');	
	*p = 0;
	return dwRet>0;
}


wstring GetIniValue(const wchar_t* strKey,const wchar_t* strFile)
{
	wchar_t ch[255];
	const int i = 255;

	GetPrivateProfileString(L"WNPD",strKey,L"",ch,i,strFile);
	wstring strValue(ch);

	size_t pos = strValue.find_last_of(L"=");
	if( pos != wstring::npos)
		strValue = strValue.substr(pos+1,strValue.length());
	return strValue;

}

BOOL GetIniInfo(WnpdInfo& info)
{
	wchar_t ads[MAX_PATH] = {0};

	GetExePath(ads);
	wcscat(ads,L"\\wnpd.ini");

	info.nginx_path = GetIniValue(L"nginx_path", ads);
	info.nginx_exe_filename = GetIniValue(L"nginx_exe_filename", ads);
	info.nginx_conf_file = GetIniValue(L"nginx_conf_file", ads);
	info.php_cgi_path = GetIniValue(L"php_cgi_path", ads);
	info.php_cgi_exe_filename = GetIniValue(L"php_cgi_exe_filename", ads);
	info.php_cgi_ip = GetIniValue(L"php_cgi_ip", ads);
	info.php_cgi_port = GetIniValue(L"php_cgi_port", ads);
	info.php_ini_file = GetIniValue(L"php_ini_file", ads);
	
	return TRUE;
}


BOOL StopPhp(const WnpdInfo& info)
{
	return KillProcess(info.php_cgi_exe_filename.c_str());
}

BOOL StartPhp(const WnpdInfo& info)
{
	HINSTANCE hInst;
	wchar_t szCmd[MAX_PATH];
	wchar_t szPara[MAX_PATH];

	swprintf(szCmd,L"%s\\%s",info.php_cgi_path.c_str(),info.php_cgi_exe_filename.c_str());
	swprintf(szPara,L"-b %s:%s",info.php_cgi_ip.c_str(),info.php_cgi_port.c_str());

	hInst = ShellExecute(NULL, L"open",	szCmd, szPara, info.php_cgi_path.c_str(), SW_HIDE);
	return ((int)hInst > 32);
}

BOOL CreateReceivePipe()
{
	SECURITY_ATTRIBUTES sa;    

	sa.bInheritHandle = TRUE;
	sa.lpSecurityDescriptor = NULL;
	sa.nLength = sizeof(SECURITY_ATTRIBUTES);

	if(!CreatePipe(&hStdOutRead,&hStdOutWrite,&sa,0))
	{
		return FALSE;
	}
	return TRUE;
}

BOOL CreatePipedProcess(const WnpdInfo& info)
{
	wchar_t szProgName[MAX_PATH];

	PROCESS_INFORMATION processInfo;
	STARTUPINFO startUpInfo = {0};

	swprintf(szProgName,L"%s\\%s -t",info.nginx_path.c_str(),info.nginx_exe_filename.c_str());

	startUpInfo.cb = sizeof(STARTUPINFO);
	startUpInfo.dwFlags =  STARTF_USESTDHANDLES;
	startUpInfo.hStdOutput = hStdOutWrite;
	startUpInfo.hStdError = hStdOutWrite;
	startUpInfo.hStdInput = NULL;
	startUpInfo.wShowWindow = SW_HIDE;


	return CreateProcess(	NULL,szProgName, 
							NULL, NULL, TRUE, 
							CREATE_NEW_CONSOLE, 
							NULL, 
							info.nginx_path.c_str(), 
							&startUpInfo, 
							&processInfo
						);
	
}

BOOL CheckConf(const WnpdInfo& info)
{
	BOOL bRet = FALSE;	

	if(!CreateReceivePipe())
		return bRet;	
	
	__try
	{
		if(!CreatePipedProcess(info))
		{
			return bRet;
		}

		char buffer[1024] = {0};  
		DWORD bytesRead = 0; 
	
		Sleep(100);
		while(ReadFile(hStdOutRead,buffer,1024,&bytesRead,NULL) && bytesRead > 0) 
		{
			if(strstr(buffer,"successful"))
			{
				bRet = TRUE;
				break;
			}
		}
	}
	__finally
	{
		CloseHandle(hStdOutWrite); 
		CloseHandle(hStdOutRead);
	}

	return bRet; 
}

BOOL StopNginx(const WnpdInfo& info)
{
	HINSTANCE hInst;
	wchar_t szCmd[MAX_PATH];

	swprintf(szCmd,L"%s\\%s",info.nginx_path.c_str(),info.nginx_exe_filename.c_str());
	hInst = ShellExecute(NULL, L"open",	szCmd, L"-s stop", info.nginx_path.c_str(), SW_HIDE);
	return ((int)hInst > 32);
}


BOOL StartNginx(const WnpdInfo& info)
{
	HINSTANCE hInst;
	wchar_t szCmd[MAX_PATH];
	//wchar_t szPara[MAX_PATH];

	swprintf(szCmd,L"%s\\%s",info.nginx_path.c_str(),info.nginx_exe_filename.c_str());
	//swprintf(szPara,L"-c %s",info.nginx_conf_file.c_str());

	hInst = ShellExecute(NULL, L"open",	szCmd, NULL, info.nginx_path.c_str(), SW_HIDE);
	return ((int)hInst > 32);
}

BOOL KillNginx(const WnpdInfo& info)
{
	return KillProcess(info.nginx_exe_filename.c_str());
}

void HandleFileError(const vector<wstring>& strErrFiles)
{
	wstring strErrMsg = L"下列文件不存在，或 wnpd.ini 配置的路径不正确：\n";
	for(int i=0;i<strErrFiles.size();++i)
	{
		strErrMsg.append(strErrFiles[i]);
	}

	MessageBox(NULL,strErrMsg.c_str(),L"有错误发生！",MB_OK|MB_ICONERROR);
}
BOOL VeryfyIniConfig(const WnpdInfo& info)
{
	wstring strPath;
	vector<wstring> strFileList;

	strPath = info.nginx_path + L"\\" + info.nginx_exe_filename;
	if(!CheckFileExist(strPath))
	{
		strFileList.push_back(strPath);
	}

	strPath = info.nginx_conf_file;
	if(!CheckFileExist(strPath))
	{
		strFileList.push_back(strPath);
	}

	strPath = info.php_cgi_path + L"\\" +info.php_cgi_exe_filename;
	if(!CheckFileExist(strPath))
	{
		strFileList.push_back(strPath);
	}
	if(strFileList.size()>0)
	{
		HandleFileError(strFileList);
	}
	return strFileList.size() == 0;
}

bool CheckFileExist( const wstring & strPath )
{
	WIN32_FIND_DATA FindFileData;
	HANDLE hFind;
	bool ret;
	hFind = FindFirstFile(strPath.c_str(), &FindFileData);
	ret = (hFind != INVALID_HANDLE_VALUE);
	FindClose(hFind);
	return ret;
}

BOOL StartNginxPhpService(const WnpdInfo& wInfo)
{
	if(!StartNginx(wInfo))
	{
		MessageBox(NULL,L"打开 nginx 服务器失败，请查看Log文件确定失败原因！",L"错误！",MB_OK|MB_ICONERROR);
		return FALSE;
	}
	else
	{
		StopPhp(wInfo);
		if(!StartPhp(wInfo))
		{
			MessageBox(NULL,L"打开 php-cgi 服务器失败！",L"错误！",MB_OK|MB_ICONERROR);
		}
		Sleep(100);
	}
	return TRUE;
}
