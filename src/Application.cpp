#include "Application.h"
#include "resource.h"
#define _WIN32_IE 0x301
#include <CommCtrl.h>
#include <exception>
#include <fstream>

Application::Application() {
	m_hInstance = NULL;
	m_MainWindow = NULL;
}

Application::~Application() {

}

void Application::InitInstance() {
	INITCOMMONCONTROLSEX icc;
	icc.dwICC = ICC_LISTVIEW_CLASSES | ICC_BAR_CLASSES | ICC_PROGRESS_CLASS;
	icc.dwSize = sizeof(INITCOMMONCONTROLSEX);

	InitCommonControlsEx(&icc);
}

HINSTANCE Application::GetInstance() const {
	return m_hInstance;
}

MainWindow& Application::GetMainWindow() const {
	return *m_MainWindow;
}

const char* Application::GetRemoteDir() const {
	return m_remoteDir.c_str();
}

const char* Application::GetLocalDir() const {
	return m_localDir.c_str();
}

DWORD WINAPI CopyUpdatedFilesThread(LPVOID param) {
	MainWindow& mainWindow = *reinterpret_cast<MainWindow*>(param);

	int copyedFileCount = 0;
	const char* remoteDir = mainWindow.GetApp().GetRemoteDir();
	const char* localDir = mainWindow.GetApp().GetLocalDir();
	std::string message;

	// Create directores
	const StringList& directoryList = mainWindow.GetDirectoryList();
	const StringList& fileList = mainWindow.GetUpdatedFileList();

	StringList::const_iterator iter = directoryList.begin();
	for ( ; iter != directoryList.end(); ++iter) {
		std::string dir = localDir;
		dir += *iter;
		if (!IsDir(dir.c_str())) {
			CreateDirectory(dir.c_str(), NULL);
			message = "Создание каталога: ";
			message += localDir;
			mainWindow.AddMessage(message.c_str());
		}
	}

	mainWindow.SetProgressMax(fileList.size());

	// Create files
	iter = fileList.begin();
	for ( ; iter != fileList.end(); ++iter) {
		std::string relativeFileName = *iter;
		std::string localFileName = localDir;
		std::string remoteFileName = remoteDir;

		localFileName += relativeFileName;
		remoteFileName += relativeFileName;
		message = "Копирование файла: ";
		message += relativeFileName;
		if (CopyFile(remoteFileName.c_str(), localFileName.c_str(), FALSE)) {
			mainWindow.AddMessage(message.c_str());
			++copyedFileCount;
		}
		else {
			message += " Failed";
			mainWindow.AddMessage(message.c_str());
		}

		mainWindow.StepIt();
#ifdef _DEBUG
		Sleep(1000);
#endif
	}

	mainWindow.EnableCloseItem();
	mainWindow.AddMessage("Обновление прошло успешно");

	return 0;
}

bool Application::UpdateTargetApp() {
	char buf[MAX_PATH];
	std::string remoteDir;
	std::string localDir = getExeDir();
	std::string confFileName = getExeDir();
	confFileName += SZ_CONF_FILE_NAME;
	if (!GetPrivateProfileString("main", "remote_dir", "", buf, MAX_PATH, confFileName.c_str())) {
		// throw "Не удалось прочитать значение удаленной директории";
		return false;
	}
	remoteDir = buf;

	Trim(&remoteDir);
	if (!remoteDir.empty() && !IsPathSeparator(remoteDir[remoteDir.length() - 1])) {
		remoteDir += PATH_SEPARATOR;
	}

	m_remoteDir = remoteDir;
	m_localDir = localDir;

	StringList directoryList;
	StringList fileList = FillFileListFromRemoteDir(remoteDir, directoryList);
	if (fileList.empty()) {
		return false;
	}

	// Find new (updated) files
	StringList updatedFileList = GetUpdatedFileList(fileList, remoteDir, localDir);
	if (updatedFileList.empty()) {
		return false;
	}

	bool res = true;
	try {
		m_MainWindow = new MainWindow(*this, updatedFileList, directoryList);

		SetDlgItemText(m_MainWindow->GetHwnd(), IDC_EDT_REMOTE_DIR, remoteDir.c_str());
		SetDlgItemText(m_MainWindow->GetHwnd(), IDC_EDT_LOCAL_DIR, localDir.c_str());

		ResizeWindow(m_MainWindow->GetHwnd());

		// copy updated files ...
		DWORD threadId = 0;
		CreateThread(NULL, 0, CopyUpdatedFilesThread, m_MainWindow, 0, &threadId);

		// m_MainWindow->Hande();

		MSG msg;
		HWND hWnd = m_MainWindow->GetHwnd();
		while (GetMessage(&msg, NULL, 0, 0)) {
			if (!IsDialogMessage(hWnd, &msg)) {
				DispatchMessage(&msg);
				TranslateMessage(&msg);
			}
		}
	}
	catch (...) {

		res = false;
	}

	if (m_MainWindow) {
		delete m_MainWindow;
	}

	return res;
}

int Application::Run() {
	int res = 0;

	UpdateTargetApp();

	std::string confFileName = getExeDir();
	confFileName += SZ_CONF_FILE_NAME;

	// exe_file_name
	char exeFileName[MAX_PATH];
	char commandLine[2048];
	exeFileName[0] = 0;
	GetPrivateProfileString("main", "exe_file_name", "", exeFileName, MAX_PATH, confFileName.c_str());
	commandLine[0] = 0;
	GetPrivateProfileString("main", "exe_command_line", "", commandLine, MAX_PATH, confFileName.c_str());

	if (IsFile(exeFileName)) {
		STARTUPINFO si = {0};
		PROCESS_INFORMATION pi = {0};

		std::string exeFilePath = getExeDir();
		exeFilePath += exeFileName;
		CreateProcess(exeFilePath.c_str(), commandLine, NULL, NULL,
			FALSE, 0, NULL, NULL, &si, &pi);
	}

	return res;
}

// dir include '/' on end
static void FindFilesInDir(StringList* fileList, const std::string& dir,
	const std::string& remoteDir, std::string& relativeDir,
	StringList& directoryList) {

	// Add star: /dir/ => /dir/*
	std::string pattern = dir + "*";
	WIN32_FIND_DATA wfd;

	HANDLE hFindFile = FindFirstFile(pattern.c_str(), &wfd);
	if (hFindFile) {
		while (FindNextFile(hFindFile, &wfd)) {
			if (!strcmp(wfd.cFileName, ".") || !strcmp(wfd.cFileName, "..")) {
				continue;
			}
			if (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
				std::string dir = remoteDir;
				std::string newDir;
				if (!(relativeDir).empty()) {
					newDir = relativeDir + PATH_SEPARATOR;
				}
				newDir += wfd.cFileName;
				newDir += PATH_SEPARATOR;

				dir += newDir;

				// relativeDir + PATH_SEPARATOR +
				directoryList.push_back(newDir);

				size_t lenBefore = relativeDir.length();
				size_t len = strlen(wfd.cFileName) + 1;
				relativeDir += wfd.cFileName;
				relativeDir += PATH_SEPARATOR;

				FindFilesInDir(fileList, dir, remoteDir, relativeDir, directoryList);

				relativeDir.erase(lenBefore, len);

				continue;
			}
			std::string relativeFileName = relativeDir + wfd.cFileName;
			fileList->push_back(relativeFileName);
		}

		FindClose(hFindFile);
	}
}

StringList Application::FillFileListFromRemoteDir(const std::string& remoteDir, StringList& directoryList) {
	StringList fileList;
	std::string relativeDir = "";

	FindFilesInDir(&fileList, remoteDir, remoteDir, relativeDir, directoryList);

	return fileList;
}

StringList Application::GetUpdatedFileList(const StringList& fileList, const std::string& remoteDir,
	const std::string& localDir) {

	StringList updatedFileList;

	StringList::const_iterator iter = fileList.begin();
	for ( ; iter != fileList.end(); ++iter) {
		const std::string& relativeFileName = *iter;
		std::string localFileName = localDir + relativeFileName;
		std::string remoteFileName = remoteDir + relativeFileName;

		if (!IsFile(localFileName.c_str()) || IsNewerFile(localFileName, remoteFileName)) {
			updatedFileList.push_back(relativeFileName);
		}
	}

	return updatedFileList;
}
