/*
  global.cpp
*/
#include "global.h"
#include <string.h>
#include <Windows.h>
#include <TlHelp32.h>
#include <shlobj.h>
#include <objidl.h>

extern FILE *g_fileLog;

bool isPathSeparator(const char c) {
	return (c == '\\' || c == '/');
}

const char* getExeDir() {
	static char szPath[MAX_PATH];

	GetModuleFileName(NULL, szPath, MAX_PATH);
	char *p = &szPath[strlen(szPath)];
	while (!(*p == '/' || *p == '\\'))
		--p;
	*(++p) = '\0';

	return szPath;
}

char const* getFileName(char const *szFileName) {
	if (!szFileName)
		return NULL;

	char const* p = &szFileName[strlen(szFileName)];
	while (szFileName != p && !(*p == '\\' || *p == '/'))
		--p;
	if (*p == '\\' || *p == '/')
		return p + 1;

	return NULL;
}

char* getDesktopPath(char *szLinkPath) {
	if (!szLinkPath)
		return NULL;

	HRESULT hres;
	hres = SHGetFolderPath(0, CSIDL_DESKTOPDIRECTORY, 0, 0, szLinkPath);
	if (SUCCEEDED(hres))
		return szLinkPath;

	return NULL;
}

void checkAppDataDir() {
	char szDir[MAX_PATH];

	getAppDataDir(szDir);

	if ((DWORD)-1 == GetFileAttributes(szDir)) {
		if (!CreateDirectory(szDir, NULL))
			print_err("Не удалось создать папку %s\n", szDir);
	}
}

/*
 * Retrieve %APPDATA% string
 */
bool getAppDataDir(char *szDir) {
	const char *szSubKey = "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders";
	DWORD dwDisp;
	HKEY  hKey = NULL;

	szDir[0] = 0;
	if (ERROR_SUCCESS == RegCreateKeyEx(HKEY_CURRENT_USER, szSubKey, 0, NULL,
		REG_OPTION_NON_VOLATILE, KEY_QUERY_VALUE, NULL, &hKey, &dwDisp))
	{
		return false;
	}

	bool res = true;
	DWORD size = MAX_PATH;
	if (ERROR_SUCCESS == RegQueryValueEx(hKey, "AppData", 0, NULL, (BYTE*)szDir, &size)) {
		validateDirName(szDir);
		strcat(szDir, SZ_EXE_NAME"\\");
	}
	else {
		res = false;
	}
	RegCloseKey(hKey);

	return res;
}

char* getConfFilePath(char *szIniFilePath) {
	getAppDataDir(szIniFilePath);

	return strcat(szIniFilePath, SZ_CONF_FILE_NAME);
}

char* getLogFilePath(char *szFile) {
	getAppDataDir(szFile);

	return strcat(szFile, SZ_LOG_FILE_NAME);
}

/*
 * Params:
 *   szExeName -- name of exe
 * Return:
 *   true if programm are runing
 */
bool isProgrammRuning(const char *szExeName) {
	if (!szExeName)
		return false;

	DWORD dwProcessId = GetCurrentProcessId();
	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, dwProcessId);
	if (INVALID_HANDLE_VALUE == hSnapshot) {
		return false;
	}

	PROCESSENTRY32 pe32 = {0};
	pe32.dwSize = sizeof(PROCESSENTRY32);
	if (Process32First(hSnapshot, &pe32)) {
		do {
			if (!strcmp(szExeName, pe32.szExeFile))
				return true;
		}
		while (Process32Next(hSnapshot, &pe32));
	}
	CloseHandle(hSnapshot);

	return false;
}

char* validateDirName(char *szDir) {
	if (!szDir || szDir[0] == 0)
		return NULL;

	char *p = &szDir[strlen(szDir) - 1];
	if (!isPathSeparator(*p)) {
		p[1] = PATH_SEPARATOR;
		p[2] = 0;
	}

	return szDir;
}

bool isFile(const char *szFileName) {
	DWORD dw = GetFileAttributes(szFileName);

	return (dw != 0xFFFFFFFF && !(dw & FILE_ATTRIBUTE_DIRECTORY));
}

char* vformat(const char *format, char *buf, va_list args) {
   vsprintf(buf, format, args);

   return buf;
}

void print(const char *format, ...) {
	char buf[300];

    va_list va;
	va_start(va, format);

	vformat(format, buf, va);
    printf(buf);
	fprintf(g_fileLog, buf);

	va_end(va);
}

void print_err(const char *format, ...) {
	char buf[300];

	va_list va;
	va_start(va, format);

	HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	csbi.dwCursorPosition.X = 0;
	csbi.dwCursorPosition.Y = 0;
	GetConsoleScreenBufferInfo(h, &csbi);
	SetConsoleTextAttribute(h, FOREGROUND_RED);
	vformat(format, buf, va);
	printf(buf);
	fprintf(g_fileLog, buf);
	SetConsoleTextAttribute(h, csbi.wAttributes);

	va_end(va);
}

void print_warn(const char *format, ...) {
	char buf[300];

	va_list va;
	va_start(va, format);

	HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	csbi.dwCursorPosition.X = 0;
	csbi.dwCursorPosition.Y = 0;
	GetConsoleScreenBufferInfo(h, &csbi);
	SetConsoleTextAttribute(h, FOREGROUND_GREEN | FOREGROUND_RED);
	vformat(format, buf, va);
	printf(buf);
	fprintf(g_fileLog, buf);
	SetConsoleTextAttribute(h, csbi.wAttributes);

	va_end(va);
}

int compareFileByDate(const char *szFileName1, const char *szFileName2) {
	FILETIME time1, time2;

	HANDLE hFile = CreateFile(szFileName1, 0, FILE_SHARE_READ, NULL, OPEN_EXISTING,
		FILE_ATTRIBUTE_READONLY, NULL);
	if (INVALID_HANDLE_VALUE != hFile) {
		GetFileTime(hFile, NULL, NULL, &time1);
		CloseHandle(hFile);
	}
	else {
		time1.dwLowDateTime = 0;
		time1.dwHighDateTime = 0;
	}

	hFile = CreateFile(szFileName2, 0, FILE_SHARE_READ, NULL, OPEN_EXISTING,
		FILE_ATTRIBUTE_READONLY, NULL);
	if (INVALID_HANDLE_VALUE != hFile) {
		GetFileTime(hFile, NULL, NULL, &time2);
		CloseHandle(hFile);
	}
	else {
		time2.dwLowDateTime = 0;
		time2.dwHighDateTime = 0;
	}

	return CompareFileTime(&time1, &time2);
}

char* fileTimeToString(FILETIME *pFileTime, char *szTime) {
	SYSTEMTIME st, stLocal = {0};

	if (FileTimeToSystemTime(pFileTime, &st)) {
		SystemTimeToTzSpecificLocalTime(NULL, &st, &stLocal);
		sprintf(szTime, "%4d/%02d/%02d %02d:%02d:%02d", stLocal.wYear, stLocal.wMonth, stLocal.wDay,
			stLocal.wHour, stLocal.wMinute, stLocal.wSecond);
	}
	else
		print_warn("0000/00/00 00:00:00\n");

	return szTime;
}