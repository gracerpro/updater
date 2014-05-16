#include "MainWindow.h"
#include "resource.h"
#include <CommCtrl.h>

static MainWindow* g_MainWindow = NULL;

MainWindow::MainWindow(Application& app, const StringList& updatedFileList, const StringList& directoryList, bool visible)
	: m_Application(app), m_updatedFileList(updatedFileList), m_directoryList(directoryList)
{
	m_hWnd = NULL;
	g_MainWindow = this;
	m_visibleMsecCount = 10000; // in ms

	int flags = WS_OVERLAPPEDWINDOW;
	if (visible) {
		flags |= WS_VISIBLE;
	}

	HWND hWnd = CreateDialog(app.GetInstance(), MAKEINTRESOURCE(IDR_MAINFRAME), NULL, (DLGPROC)MainWindow::MainWindowProc);
	if (!hWnd) {
		throw "Couldn't create main window";
	}

	ShowWindow(hWnd, SW_SHOW);
	UpdateWindow(hWnd);
	//ResizeWindow(hWnd);

	m_hWnd = hWnd;
}

MainWindow::~MainWindow() {

}

LRESULT CALLBACK MainWindow::MainWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {

	switch (uMsg)
	{
	case WM_COMMAND:
		int id;

		id = LOWORD(wParam);
		if (id == IDOK || id == IDCANCEL) {
			DestroyWindow(hWnd);
		}
		break;
	case WM_INITDIALOG:
		g_MainWindow->m_hWnd = hWnd;
		g_MainWindow->SetProgressStep();
		SetTimer(hWnd, 0, g_MainWindow->GetVisibleMsecCount(), NULL);
		g_MainWindow->EnableCloseItem(false);
		return TRUE;
	case WM_TIMER:
		KillTimer(hWnd, 0);
		g_MainWindow->EnableCloseItem();
		DestroyWindow(g_MainWindow->GetHwnd());
		break;
	case WM_SIZE:
		if (g_MainWindow)
			g_MainWindow->OnSize(LOWORD(lParam), HIWORD(lParam), wParam);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	}

	return 0;
}

void MainWindow::OnSize(int cx, int cy, int flags) {
	RECT rc;
	int height;
	HWND hwndProgress = GetDlgItem(m_hWnd, IDC_PGB_MAIN);
	GetWindowRect(hwndProgress, &rc);

	height = rc.bottom - rc.top;
	MoveWindow(hwndProgress, 0, cy - height, cx, height, TRUE);

	HWND hwndList = GetDlgItem(m_hWnd, IDC_LST_MESSAGE);
	MoveWindow(hwndList, 0, 90, cx, cy - 90 - height, TRUE);
}

void MainWindow::AddMessage(const char* message) {
	HWND hwndList = GetDlgItem(m_hWnd, IDC_LST_MESSAGE);
	SendMessage(hwndList, LB_ADDSTRING, 0, (LPARAM)message);
}

void MainWindow::SetProgressPos(size_t pos) {
	HWND hwndProgress = GetDlgItem(m_hWnd, IDC_PGB_MAIN);
	SendMessage(hwndProgress, PBM_SETPOS, (WPARAM)pos, 0);
}

void MainWindow::SetProgressMax(size_t max) {
	HWND hwndProgress = GetDlgItem(m_hWnd, IDC_PGB_MAIN);
	SendMessage(hwndProgress, PBM_SETRANGE, 0, MAKELPARAM(0, max));
}

void MainWindow::SetProgressStep(size_t step) {
	HWND hwndProgress = GetDlgItem(m_hWnd, IDC_PGB_MAIN);
	SendMessage(hwndProgress, PBM_SETSTEP, (WPARAM)step, 0);
}

void MainWindow::StepIt() {
	HWND hwndProgress = GetDlgItem(m_hWnd, IDC_PGB_MAIN);
	SendMessage(hwndProgress, PBM_STEPIT, 0, 0);
}

void MainWindow::EnableCloseItem(bool enable) {
	HMENU hMenu = GetSystemMenu(m_hWnd, FALSE);

	int flags = enable ? MF_BYCOMMAND & !MF_GRAYED : MF_BYCOMMAND | MF_GRAYED;

	EnableMenuItem(hMenu, SC_CLOSE, flags);
}

const StringList& MainWindow::GetDirectoryList() const {
	return m_directoryList;
}

const StringList& MainWindow::GetUpdatedFileList() const {
	return m_updatedFileList;
}

Application& MainWindow::GetApp() const {
	return m_Application;
}

HWND MainWindow::GetHwnd() const {
	return m_hWnd;
}

size_t MainWindow::GetVisibleMsecCount() const {
	return m_visibleMsecCount;
}
