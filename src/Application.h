#pragma once

#include "global.h"
#include "MainWindow.h"
#include <Windows.h>


class MainWindow;

class Application {
public:
	Application();
	~Application();

	void InitInstance();
	int Run();
	HINSTANCE GetInstance() const;

	MainWindow& GetMainWindow() const;

	const char* GetRemoteDir() const;
	const char* GetLocalDir() const;

private:
	HINSTANCE   m_hInstance;
	MainWindow* m_MainWindow;
	std::string m_localDir;
	std::string m_remoteDir;

	bool UpdateTargetApp();

	StringList FillFileListFromRemoteDir(const std::string& remoteDir, StringList& directoryList); // out directoryList
	StringList GetUpdatedFileList(const StringList& fileList, const std::string& remoteDir, const std::string& localDir);
};