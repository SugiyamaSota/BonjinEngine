#include"WinApp.h"

#pragma comment(lib,"winmm.lib")

#include <dbghelp.h>
#include <strsafe.h>

#include "../../externals/imgui/imgui_impl_win32.h"
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);


WinApp* WinApp::instance_ = nullptr;

WinApp::WinApp() {
}

WinApp* WinApp::GetInstance() {
	if (instance_ == nullptr) {
		instance_ = new WinApp();
		instance_->Initialize();
	}
	return instance_;
}

void WinApp::DestroyInstance() {
	delete instance_;
	instance_ = nullptr;
}

void WinApp::Initialize() {
	// ログのディレクトリを用意
	std::filesystem::create_directory("logs");

	// ダンプファイルの設定
	SetUnhandledExceptionFilter(ExportDump);

	// 必要な奴
	HRESULT hr = CoInitializeEx(0, COINIT_MULTITHREADED);

	// ウィンドウクラスの生成と登録
	WNDCLASS wc{};
	wc.lpfnWndProc = WindowProc;
	wc.lpszClassName = L"CG2WindowClass";
	wc.hInstance = GetModuleHandle(nullptr);
	hInstance_ = wc.hInstance; // インスタンスハンドルを保持
	wc.hCursor = (LoadCursor(nullptr, IDC_ARROW));
	RegisterClass(&wc);

	//
	RECT wrc = { 0,0,kClientWidth,kClientHeight };
	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);

	//ウィンドウの生成と表示
	hwnd_ = CreateWindow(
		wc.lpszClassName,
		L"CG2",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		wrc.right - wrc.left,
		wrc.bottom - wrc.top,
		nullptr,
		nullptr,
		wc.hInstance,
		nullptr);
	ShowWindow(hwnd_, SW_SHOW);

	//
	timeBeginPeriod(1);
}

WinApp::~WinApp() {
	CoUninitialize();
	CloseWindow(hwnd_);
}

bool WinApp::ProcessMessage() {
	MSG msg{};

	if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	if (msg.message == WM_QUIT)
	{
		return true;
	}

	return false;
}


LRESULT CALLBACK WinApp::WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
#ifdef USE_IMGUI
	if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam)) {
		return true;
	}
#endif
	// メッセージに応じてゲーム固有の処理を行う
	switch (msg) {
	case WM_KEYUP: // キーが離されたとき
		// Escキーが押されたら
		if (wparam == VK_ESCAPE) {
			// Windowを閉じるメッセージを送信する
			PostMessage(hwnd, WM_CLOSE, 0, 0);
		}
		return 0;
		// ウィンドウが破棄された
	case WM_DESTROY:
		// OSに対して、アプリの終了を伝える
		PostQuitMessage(0);
		return 0;
	}

	// 標準のメッセージ処理を行う
	return DefWindowProc(hwnd, msg, wparam, lparam);
}

LONG WINAPI WinApp::ExportDump(EXCEPTION_POINTERS* exception) {
	//時刻を取得して、時刻を名前に入れたファイルを作成。Dumpsディレクトリ以下に出力
	SYSTEMTIME time;
	GetLocalTime(&time);
	wchar_t filePath[MAX_PATH] = { 0 };
	CreateDirectory(L"./Dumps", nullptr);
	StringCchPrintfW(filePath, MAX_PATH, L"./Dumps/%04d-%02d%02d-%02d%02d.dmp", time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute);
	HANDLE dumpFileHandle = CreateFile(filePath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_WRITE | FILE_SHARE_READ, 0, CREATE_ALWAYS, 0, 0);
	//processID(このexeのid)とクラッシュの発生したthreadidを取得
	DWORD processId = GetCurrentProcessId();
	DWORD threadId = GetCurrentThreadId();
	//設定情報を入力
	MINIDUMP_EXCEPTION_INFORMATION minidumpInformation{ 0 };
	minidumpInformation.ThreadId = threadId;
	minidumpInformation.ExceptionPointers = exception;
	minidumpInformation.ClientPointers = TRUE;
	//Dumpを出力。MiniDumpNormalは最低限の情報を出力するフラグ
	MiniDumpWriteDump(GetCurrentProcess(), processId, dumpFileHandle, MiniDumpNormal, &minidumpInformation, nullptr, nullptr);
	//ほかに関連付けられているSEH例外ハンドラがあれば実行。通常はprocessを終了する
	return EXCEPTION_EXECUTE_HANDLER;
}

