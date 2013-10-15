/*
  global.h
*/
#ifndef _GLOBAL_H
#define _GLOBAL_H

#include <stdlib.h>
#include <stdio.h>
#include <conio.h>
#include <Windows.h>

#define PATH_SEPARATOR      '\\'

#define SZ_PAR_REMOTE_DIR   "remote_dir"
#define SZ_PAR_EXE_NAME     "exe_file_name"
#define SZ_PAR_LINK_NAME    "link_name"
#define SZ_PAR_LINK_COMMENT "link_comment"
#define SZ_PAR_RUN_APP_COPY "run_app_copy"
#define SZ_PAR_SHOW_MESSAGE "show_update_message"

#define SZ_LOADER_FILE_FILES   "loader_files" // file name of file with list copied files
#define SZ_LOADER_FILE_MESSAGE "loader_message"
#define SZ_EXE_NAME            "loader"
#define SZ_EXE_FILE_NAME       "loader.exe"
#define SZ_CONF_FILE_NAME      "loader.ini"
#define SZ_LOG_FILE_NAME       "loader.log"
#define SZ_CONF_EXT            "ini"
#define SZ_EVENT_COPY_FILE     "event_copy_file"

#define SZ_PRESS_ANY_KEY   "Нажмите любую клавишу для продолжения...\n"

//#define print(str, ...) printf(str, ##__VA_ARGS__)

#define IsKeyPressed(key) (GetKeyState(key) & (1 << (sizeof(SHORT) * 8 - 1))) > 0

void print(char const *format, ...);
void print_err(char const *format, ...);
void print_warn(char const *format, ...);

bool         getAppDataDir(char *szDir);
void         checkAppDataDir();
char*        getConfFilePath(char *szIniFilePath);
char*        getLogFilePath(char *szFilePath);
char*        getDesktopPath(char *szLinkPath);
char const*  getFileName(char const *szFileName);
char const*  getExeDir();
char*        validateDirName(char *szDir);

bool         isPathSeparator(const char c);
bool         isProgrammRuning(char const *szExeName);
bool         isFile(char const *szFileName);

int          compareFileByDate(char const *szFileName1, char const *szFileName2);
char*        fileTimeToString(FILETIME *pFileTime, char *szTime);

#endif