#pragma once

#include "resource.h"
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <shellapi.h>
#include <string>
#include <process.h>
#include <Psapi.h>
#include <vector>


using namespace std;

#pragma comment(lib,"psapi.lib")

struct WnpdInfo
{
	wstring nginx_path;
	wstring nginx_exe_filename;
	wstring nginx_conf_file;
	wstring php_cgi_path;
	wstring php_cgi_exe_filename;
	wstring php_cgi_ip;
	wstring php_cgi_port;
	wstring php_ini_file;
};


class CCritSec  
{
public:
	CCritSec();
	virtual ~CCritSec();
public:
	void Lock(void);
	void Unlock(void);
protected:
	CRITICAL_SECTION m_CritSec;
};

BOOL GetExePath( wchar_t *szUrlPath ,int len = MAX_PATH );
BOOL StartNginx(const WnpdInfo& info);
BOOL StopNginx(const WnpdInfo& info);
BOOL GetIniInfo(WnpdInfo& info);
BOOL CheckConf(const WnpdInfo& info);
BOOL KillNginx(const WnpdInfo& info);
BOOL StopPhp(const WnpdInfo& info);
BOOL StartPhp(const WnpdInfo& info);
BOOL VeryfyIniConfig(const WnpdInfo& info);
bool CheckFileExist( const wstring & strPath );
BOOL StartNginxPhpService(const WnpdInfo& wInfo);