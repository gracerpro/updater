/*
  Loader.cpp
*/
#include "global.h"
#include <io.h>
#include <clocale>
#include <shlobj.h>
#include <objidl.h>

/*
  source dir/
            /files
*/

bool    isNewVersionExists(FILETIME *pTimeLocal, FILETIME *pTimeRemote);
int     copyDistributive();
bool    showNewVersionMessage(int nChangeFileCount);
BOOL    createLinkOnDesktop(char const *szExeName, char const *szLinkName, int nShowCmd, char const *szComment);
BOOL    runApplication(char const *szFile);
HANDLE  isLoaderRining(); // Return NULL if application are runing

char g_szRemoteDir[MAX_PATH];
char g_szExeFileNameRemote[MAX_PATH];
char g_szExeFileNameLocal[MAX_PATH];
int  g_nKeyWaitTime = 0;
FILE *g_fileLog = NULL;
HANDLE g_hMutex = NULL;

/*
 * Params:
 *   szExeFile -- absolute name of exe
 * Return:
 *   FALSE if function fail
 */
BOOL runApplication(const char *szExeFile) {
	char szConfFile[MAX_PATH];
	char buf[50];
	int  n = 0;

	if (!isFile(szExeFile))
		return FALSE;

	getConfFilePath(szConfFile);
	if (GetPrivateProfileString("main", SZ_PAR_RUN_APP_COPY, "0", buf, MAX_PATH, szConfFile))
		n = atoi(buf);

	if (!n) {
		if (isProgrammRuning(getFileName(szExeFile))) {
			print_warn("Приложение уже запущено!\n");
			print(SZ_PRESS_ANY_KEY);
			return FALSE;
		}
	}

	STARTUPINFO si = {0};
	PROCESS_INFORMATION pi = {0};
	si.cb = sizeof(STARTUPINFO);

	return CreateProcess(g_szExeFileNameLocal, NULL, NULL, NULL, FALSE, CREATE_DEFAULT_ERROR_MODE,
		NULL, NULL, &si, &pi);
}

BOOL createLinkOnDesktop(char const *szFileName, char const *szLinkName, int nShowCmd,
	char const *szComment)
{
	HRESULT hres;
	IShellLink *NewLink;

	CoInitialize(NULL);
	hres = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLink,
		(void **)&NewLink);
	if (SUCCEEDED(hres)) {
		NewLink->SetPath(szFileName);
		NewLink->SetWorkingDirectory(getExeDir());
		if (!szComment)
			szComment = "";
		NewLink->SetDescription(szComment);
		NewLink->SetArguments("");
		NewLink->SetShowCmd(nShowCmd);
		NewLink->SetIconLocation(g_szExeFileNameLocal, 0);
		IPersistFile *ppf;
		hres = NewLink->QueryInterface(IID_IPersistFile, (void**)&ppf);
		if (SUCCEEDED(hres)) {
			wchar_t wsz[MAX_PATH];
			MultiByteToWideChar(CP_ACP, 0, szLinkName, -1, wsz,  MAX_PATH);
			hres = ppf->Save(wsz, TRUE);
		}
	}
	CoUninitialize();

	return TRUE;
}

bool isNewVersionExists(FILETIME *pTimeLocal, FILETIME *pTimeRemote) {
	HANDLE hFile;
	FILETIME timeLocal, timeRemote;

	if (!g_szExeFileNameLocal[0])
		return true;

	hFile = CreateFile(g_szExeFileNameLocal, 0, FILE_SHARE_READ, NULL, OPEN_EXISTING,
		FILE_ATTRIBUTE_READONLY, NULL);
	GetFileTime(hFile, NULL, NULL, &timeLocal);
	if (pTimeLocal)
		*pTimeLocal = timeLocal;
	CloseHandle(hFile);

	hFile = CreateFile(g_szExeFileNameRemote, 0, FILE_SHARE_READ, NULL, OPEN_EXISTING,
		FILE_ATTRIBUTE_READONLY, NULL);
	GetFileTime(hFile, NULL, NULL, &timeRemote);
	if (pTimeRemote)
		*pTimeRemote = timeRemote;
	CloseHandle(hFile);

	return (CompareFileTime(&timeLocal, &timeRemote) < 0);
}

DWORD WINAPI threadCopyFile(LPVOID lpParam) {
	char szFileNameRemote[MAX_PATH];
	char szFileNameLocale[MAX_PATH];
	char buf[MAX_PATH];
	char *pFileLocale, *pFileRemote;
	bool bForce = lpParam != NULL;

	strcpy(szFileNameRemote, g_szRemoteDir);
	pFileRemote = &szFileNameRemote[strlen(szFileNameRemote)];
	strcat(szFileNameRemote, SZ_LOADER_FILE_FILES);

	strcpy(szFileNameLocale, getExeDir());
	pFileLocale = &szFileNameLocale[strlen(szFileNameLocale)];

	// Copy the each file and print warnings
	FILE *f = fopen(szFileNameRemote, "rt");
	if (!f) {
		print_err("Не удалось открыть файл!\n");
		print("%s", szFileNameRemote);
		ExitThread(0);
	}

	size_t fileCount = 0, fileCountSuccess = 0;
	while (fgets(buf, MAX_PATH, f)) {
		char *pCR = strrchr(buf, '\n');
		if (pCR)
			*pCR = 0;
		if (!buf[0]) // skip empty lines
			continue;

		//Sleep(1000);
		// szFileFiles = DIR/ + file_name
		strcpy(pFileRemote, buf);
		strcpy(pFileLocale, buf);
		if (!isFile(szFileNameRemote)) {
			print_warn("Файл из списка не найден в удаленной папке\n");
			print("%s\n", pFileRemote);
			++fileCount;
			continue;
		}
		// If file is not changed?
		if (!bForce && compareFileByDate(szFileNameLocale, szFileNameRemote) >= 0) {
			++fileCount;
			continue;
		}

		print("Файл \"%s\"", pFileRemote);
		if (CopyFile(szFileNameRemote, szFileNameLocale, FALSE)) {
			++fileCountSuccess;
			print(" OK");
		}
		else {
			print_warn(" Не удалось скопировать файл.");
		}
		print("\n");
		++fileCount;
	}
	fclose(f);

	print("\nСкопировано файлов %d из %d\n", fileCountSuccess, fileCount);

	HANDLE hEvent = OpenEvent(EVENT_ALL_ACCESS, TRUE, SZ_EVENT_COPY_FILE);
	SetEvent(hEvent);
	CloseHandle(hEvent);

	ExitThread(fileCountSuccess);
}

int copyDistributive(bool bForceCopy) {
	char szFileNameRemote[MAX_PATH];

	strcpy(szFileNameRemote, g_szRemoteDir);
	strcat(szFileNameRemote, SZ_LOADER_FILE_FILES);

	if (!isFile(szFileNameRemote)) {
		print_err("Не удалось найти файл \""SZ_LOADER_FILE_FILES"\" в удаленной папке\n%s\n",
			szFileNameRemote);
		return 0;
	}

	DWORD dwThread;
	HANDLE hEvent = CreateEvent(NULL, FALSE, FALSE, SZ_EVENT_COPY_FILE);

	print("Копирование...\n");
	HANDLE hThread = CreateThread(NULL, 0, threadCopyFile, (LPVOID)bForceCopy, 0, &dwThread);

	while (true) {
#ifdef _DEBUG
		DWORD dw = WaitForSingleObject(hEvent, 1000);
#else
		DWORD dw = WaitForSingleObject(hEvent, 200);
#endif
		if (WAIT_OBJECT_0 == dw)
			break;
		if (IsKeyPressed(VK_ESCAPE)) {
			print_warn("Процесс копирования прерван\n");
			break;
		}
	}
	DWORD dw;
	Sleep(100);
	if (!GetExitCodeThread(hThread, &dw))
		dw = 0;
	DWORD dwPriority = GetThreadPriority(hThread);
	if (STILL_ACTIVE == dw && THREAD_PRIORITY_ERROR_RETURN != dwPriority)
		dw = 0;

	CloseHandle(hEvent);
	CloseHandle(hThread);

	return dw;
}

bool showNewVersionMessage(int nChangeFileCount) {
	char szFile[MAX_PATH];
	char szConfFile[MAX_PATH];
	char szShowMessage[50];
	bool bShowMessage = false;
	
	getConfFilePath(szConfFile);
	szShowMessage[0] = 0;
	GetPrivateProfileString("main", SZ_PAR_SHOW_MESSAGE, "1", szShowMessage, 50, szConfFile);
	bShowMessage = (atoi(szShowMessage) > 0);
	if (bShowMessage && !nChangeFileCount)
		return false;

	strcpy(szFile, g_szRemoteDir);
	strcat(szFile, SZ_LOADER_FILE_MESSAGE);
	if (!isFile(szFile)) {
		print_warn("Не найден файл с сообщением для пользователя\n");
		return false;
	}

	DWORD dw = 0;
	HANDLE hFile = CreateFile(szFile, GENERIC_READ, FILE_SHARE_READ, NULL,
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (INVALID_HANDLE_VALUE == hFile) {
		print_err("Не удалось создать файл %s\n", szFile);
		return false;
	}

	DWORD dwSize = GetFileSize(hFile, NULL);
	if (dwSize) {
		char *data = (char*)LocalAlloc(0, dwSize + 1);
		ReadFile(hFile, data, dwSize, &dw, NULL);
		data[dwSize] = 0;
		print_warn("\nОписание изменений в приложении:\n");
		print("/*********************************\n");
		char *pLine = data;
		while (char *pnl = strchr(pLine, '\n')) {
			print("* ");
			*pnl = 0;
			print(pLine);
			print("\n");
			pLine = ++pnl;
		}
		if (pLine)
			print("* %s\n*\n", pLine);
		print("*********************************/\n");
		LocalFree(data);
	}
	CloseHandle(hFile);

	return true;
}

HANDLE isLoaderRining() {
	HANDLE hMutex = CreateMutex(NULL, FALSE, "loader_mutex_name_2013_04");
	if (GetLastError() == ERROR_ALREADY_EXISTS) {
		CloseHandle(hMutex);
		return NULL;
	}

	return hMutex;
}

// TODO: run by command line
int main(int argc, char *argv[]) {
    int  res = 0;
	bool bCopyNewExe = false; // Copy new application and create the shortcut
	char szConfFile[MAX_PATH];

	setlocale(LC_ALL, "");
	checkAppDataDir();
	getLogFilePath(szConfFile);
	g_fileLog = fopen(szConfFile, "w");
	print("***************************************************\n");
	print("* Loader.exe version 1.0 created by SlaFF 2013/04 *\n");
	print("* Press \"Esc\" to break coping                     *\n");
	print("* Put \"Shift\" for force coping                    *\n");
	print("* The data value prints in yyyy/mm/dd format      *\n");
	print("* The time value prints in hh/mm/ss format        *\n");
	print("***************************************************\n\n");
/*	print("      ________                  _________  _________ ");
	print("     / ______/  /\\             /  ______/ /  ______/ ");
	print("    / /____    | |     _____   | |_____   | |_____   ");
	print("    \\_____ \\   / /    / ___ \\  |  ____/   |  ____/  _____   ");
	print("          \\ \\  | |   / /  / /  | |        | |      /  __ \\ ");
	print("   _______/ /  / /  / /__/ |   | |        | |      | |_  |  ");
	print("  /________/   \\/   \\____/\\_\\  \\_/        \\_/      \\_____/");*/

	g_hMutex = isLoaderRining();
	if (!g_hMutex) {
		print_warn("Файловый загрузчик уже запущен\n");
		fclose(g_fileLog);
		return 0;
	}

	// read params
	getConfFilePath(szConfFile);
	if (!isFile(szConfFile)) {
		print_err("Не удалось найти файл настроек \"%s\"\n", szConfFile);
		goto l_err;
	}
	if (!GetPrivateProfileString("main", SZ_PAR_REMOTE_DIR, "", g_szRemoteDir, MAX_PATH, szConfFile)) {
		print_err("Не удалось прочитать параметр \""SZ_PAR_REMOTE_DIR"\" из файла настроек\n");
		goto l_err;
	}
	if (!g_szRemoteDir[0]) {
		print_err("Параметр \""SZ_PAR_REMOTE_DIR"\" содержит пустое значение!\n");
		goto l_err;
	}
	validateDirName(g_szRemoteDir);

	char szExeName[200];
	if (!GetPrivateProfileString("main", SZ_PAR_EXE_NAME, "", szExeName, MAX_PATH, szConfFile)) {
		print_err("Не удалось прочитать параметр \""SZ_PAR_EXE_NAME"\" из файла настроек\n");
		szExeName[0] = 0;
	}
	else if (!szExeName[0]) {
		print_err("Параметр \""SZ_PAR_EXE_NAME"\" содержит пустое значение!\n");
		//goto l_err;
	}
	strcpy(g_szExeFileNameLocal, getExeDir());
	strcat(g_szExeFileNameLocal, szExeName);
	if (!isFile(g_szExeFileNameLocal)) {
		print_warn("Не удается найти обновляемое приложение!\n");
		print("%s\n", g_szExeFileNameLocal);
		g_szExeFileNameLocal[0] = 0;
		bCopyNewExe = true;
	}
	strcpy(g_szExeFileNameRemote, g_szRemoteDir);
	strcat(g_szExeFileNameRemote, szExeName);
	if (!isFile(g_szExeFileNameRemote)) {
		print_err("Не удается найти новую версию приложения!\n");
		print("%s\n", g_szExeFileNameRemote);
		g_szExeFileNameRemote[0] = 0;
		// goto l_err;
	}
	// check new version
	// Check of date of remote file with date of local file
	// Date(remote) <= Date(local) RETURN
	FILETIME timeLocale, timeRomote;
	bool bForceCopy;
	bForceCopy = IsKeyPressed(VK_SHIFT);
	if (!bForceCopy && g_szExeFileNameRemote[0] && g_szExeFileNameLocal[0]
		&& !isNewVersionExists(&timeLocale, &timeRomote))
	{
		char buf[100];
		print_warn("Версия приложения не устарела.\n");
		print("Дата старой версии %s\n", fileTimeToString(&timeLocale, buf));
		print("Дата новой версии  %s\n", fileTimeToString(&timeRomote, buf));
		WritePrivateProfileString("main", SZ_PAR_SHOW_MESSAGE, "1", szConfFile);
	}
	else {
		// check the message show
		WritePrivateProfileString("main", SZ_PAR_SHOW_MESSAGE, "0", szConfFile);
	}
	int nFileCount;
	nFileCount = copyDistributive(bForceCopy);

	print("Удаленная папка \"%s\"\nЛокальная папка \"%s\"\nПриложение \"%s\"\n",
		g_szRemoteDir, getExeDir(), g_szExeFileNameLocal);

	char szLinkName[200];
	if (!GetPrivateProfileString("main", SZ_PAR_LINK_NAME, "", szLinkName, 255, szConfFile)) {
		print_warn("Не удалось прочитать параметр "SZ_PAR_LINK_NAME" из файла конфигурации\n");
		strcpy(szLinkName, szExeName);
	}
	char szLinkComment[261];
	if (!GetPrivateProfileString("main", SZ_PAR_LINK_COMMENT, "", szLinkComment, 260, szConfFile)) {
		print_warn("Не удалось прочитать параметр "SZ_PAR_LINK_COMMENT" из файла конфигурации\n");
		szLinkComment[0] = 0;
	}
	
	char szLinkPath[MAX_PATH];
	char szLinkFileName[MAX_PATH];
	
	// Create shortcut
	getDesktopPath(szLinkPath);
	validateDirName(szLinkPath);
	strcat(szLinkPath, szLinkName);
	strcat(szLinkPath, ".lnk");
	strcpy(szLinkFileName, getExeDir());
	strcat(szLinkFileName, SZ_EXE_FILE_NAME);
	strcpy(g_szExeFileNameLocal, getExeDir());
	strcat(g_szExeFileNameLocal, szExeName);
	if (bCopyNewExe && g_szExeFileNameRemote[0]) {
		print("\nСоздание ярлыка...\n");
		createLinkOnDesktop(szLinkFileName, szLinkPath, SW_SHOWNORMAL, szLinkComment);
	}

	if (showNewVersionMessage(nFileCount)) {
		print("\n"SZ_PRESS_ANY_KEY);
		getch();
	} //*/

	if (bForceCopy) {
	    print("Press 0 -- exit\nPress 1 -- Run exe file");
	    switch (getch()) {
			case '0':
				goto l_ok;
				break;
			case '1':
				//  print("\n"SZ_PRESS_ANY_KEY);
				//  getch();
				break;
	    }
	}

	// Run the application
	if (!runApplication(g_szExeFileNameLocal))
		goto l_err;

	goto l_ok;

l_err:
	res = 1;
	getch();

l_ok:
	fclose(g_fileLog);
	ReleaseMutex(g_hMutex);
    CloseHandle(g_hMutex);

	return res;
}