#pragma once

#include <Windows.h>
#include "Application.h"

class Application;

class MainWindow {
public:
	MainWindow(Application& app, const StringList& updatedFileList, const StringList& directoryList, bool visible = true);
	~MainWindow();

	HWND GetHwnd() const;
	Application& GetApp() const;

	size_t GetVisibleMsecCount() const;

	void OnSize(int cx, int cy, int flags);

	void SetProgressPos(size_t pos = 0);
	void SetProgressMax(size_t max = 100);
	void SetProgressStep(size_t step = 1);
	void StepIt();

	void EnableCloseItem(bool enable = true);
	void AddMessage(const char* message);
	const StringList& GetUpdatedFileList() const;
	const StringList& GetDirectoryList() const;

private:
	HWND m_hWnd;
	Application& m_Application;
	const StringList& m_updatedFileList;
	const StringList& m_directoryList;
	size_t m_visibleMsecCount; // global

	static LRESULT CALLBACK MainWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	friend int MyRegisterClass(HINSTANCE);

	// warning hack
	MainWindow& operator= (const MainWindow& mainWindow) { return *this; };
};
