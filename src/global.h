/*
  global.h
*/
#pragma once

#include <Windows.h>
#include <string>
#include <vector>

typedef std::vector<std::string> StringList;


#define PATH_SEPARATOR      '\\'

#define SZ_PAR_LINK_NAME    "link_name"
#define SZ_PAR_LINK_COMMENT "link_comment"

#define SZ_EXE_NAME         "updater"
#define SZ_EXE_FILE_NAME    "updater.exe"
#define SZ_CONF_FILE_NAME   "updater.ini"
#define SZ_LOG_FILE_NAME    "updater.log"
#define SZ_CONF_EXT         "ini"

char const*  getExeDir();

bool IsKeyPressed(char key);
void ResizeWindow(HWND hWnd);

bool IsPathSeparator(const char c);
bool IsProgrammRuning(char const *szExeName);
bool IsFile(char const *szFileName);
bool IsDir(char const *szFileName);
bool IsNewerFile(const std::string& szFileName1, const std::string szFileName2);

void Trim(std::string* str);