/*
  global.cpp
*/
#include "global.h"
#include <Windows.h>

bool IsPathSeparator(const char c) {
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

bool IsFile(const char *szFileName) {
	DWORD dw = GetFileAttributes(szFileName);

	return (dw != 0xFFFFFFFF && !(dw & FILE_ATTRIBUTE_DIRECTORY));
}

bool IsDir(char const *szFileName) {
	DWORD dw = GetFileAttributes(szFileName);

	return (dw != 0xFFFFFFFF && (dw & FILE_ATTRIBUTE_DIRECTORY) > 0);
}

bool IsNewerFile(const std::string& szFileName1, const std::string szFileName2) {
	FILETIME time1, time2;

	HANDLE hFile = CreateFile(szFileName1.c_str(), 0, FILE_SHARE_READ, NULL, OPEN_EXISTING,
		FILE_ATTRIBUTE_READONLY, NULL);
	if (INVALID_HANDLE_VALUE != hFile) {
		GetFileTime(hFile, NULL, NULL, &time1);
		CloseHandle(hFile);
	}
	else {
		time1.dwLowDateTime = 0;
		time1.dwHighDateTime = 0;
	}

	hFile = CreateFile(szFileName2.c_str(), 0, FILE_SHARE_READ, NULL, OPEN_EXISTING,
		FILE_ATTRIBUTE_READONLY, NULL);
	if (INVALID_HANDLE_VALUE != hFile) {
		GetFileTime(hFile, NULL, NULL, &time2);
		CloseHandle(hFile);
	}
	else {
		time2.dwLowDateTime = 0;
		time2.dwHighDateTime = 0;
	}

	return (CompareFileTime(&time1, &time2) < 0);
}

void Trim(std::string* str) {
	size_t p = str->find_first_not_of(" \t\n");
	str->erase(0, p);

	p = str->find_last_not_of(" \t\n");
	if (std::string::npos != p)
		str->erase(p + 1);
}

void ResizeWindow(HWND hWnd) {
	RECT rc;

	if (hWnd && GetClientRect(hWnd, &rc)) {
		SendMessage(hWnd, WM_SIZE, 0, MAKELPARAM(rc.right, rc.bottom));
	}
}

bool IsKeyPressed(char key) {
	return (GetKeyState(key) & (1 << (sizeof(SHORT) * 8 - 1))) > 0;
}
