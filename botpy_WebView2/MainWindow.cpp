#include "MainWindow.h"
#include "webview_handlers.h"
#include "json.hpp"
#include "resource.h"
#include <string>

using json = nlohmann::json;

static std::wstring utf8_to_wide(const std::string& utf8) {
    if (utf8.empty()) return L"";
    int len = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, NULL, 0);
    if (len <= 0) return L"";
    std::wstring result(len, 0);
    MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, &result[0], len);
    return result;
}

MainWindow::MainWindow()
    : m_hwnd(nullptr), m_webview(nullptr), m_controller(nullptr), m_webview_ready(false) {}

MainWindow::~MainWindow() {
    if (m_controller) {
        m_controller->Release();
        m_controller = nullptr;
    }
    if (m_webview) {
        m_webview->Release();
        m_webview = nullptr;
    }
    m_log_cache.clear();
}

bool MainWindow::Create(HINSTANCE hInstance, int nCmdShow) {
    const wchar_t CLASS_NAME[] = L"MiaoBotWindow";

    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

    RegisterClass(&wc);

    m_hwnd = CreateWindowEx(
        0, CLASS_NAME, L"Miao Bot - QQ Robot Console",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 1000, 700,
        nullptr, nullptr, hInstance, this
    );

    if (!m_hwnd) return false;

    InitializeWebView();

    ShowWindow(m_hwnd, nCmdShow);
    UpdateWindow(m_hwnd);
    return true;
}

void MainWindow::Show() {
    ShowWindow(m_hwnd, SW_SHOW);
}

void MainWindow::RunMessageLoop() {
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

LRESULT CALLBACK MainWindow::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    MainWindow* pThis = nullptr;
    if (uMsg == WM_NCCREATE) {
        CREATESTRUCT* pCreate = (CREATESTRUCT*)lParam;
        pThis = (MainWindow*)pCreate->lpCreateParams;
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)pThis);
        pThis->m_hwnd = hwnd;
    } else {
        pThis = (MainWindow*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    }
    if (pThis) {
        return pThis->HandleMessage(uMsg, wParam, lParam);
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

LRESULT MainWindow::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_UILOG: {
        UiLogData* data = (UiLogData*)wParam;
        DoPostLog(data);
        delete data;
        return 0;
    }
    case WM_UISTATUS: {
        UiStatusData* data = (UiStatusData*)wParam;
        DoPostStatus(data);
        delete data;
        return 0;
    }
    case WM_UIINFO: {
        UiInfoData* data = (UiInfoData*)wParam;
        DoPostInfo(data);
        delete data;
        return 0;
    }
    case WM_UIMESSAGE: {
        UiMessageData* data = (UiMessageData*)wParam;
        DoPostMessage(data);
        delete data;
        return 0;
    }
    case WM_UIREADY: {
        if (m_ready_handler) {
            m_ready_handler();
        }
        return 0;
    }
    case WM_SIZE:
        ResizeWebView();
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    default:
        return DefWindowProc(m_hwnd, uMsg, wParam, lParam);
    }
}

void MainWindow::InitializeWebView() {


    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

    HRESULT hr = CreateCoreWebView2EnvironmentWithOptions(
        nullptr, nullptr, nullptr,
        new CreateEnvironmentCompletedHandler(this));
    
}

void MainWindow::SetWebView(ICoreWebView2* webview, ICoreWebView2Controller* controller) {
    m_webview = webview;
    m_controller = controller;
}

void MainWindow::OnReady() {
    // WebView2 controller is ready, but page navigation may still be in progress
    // Wait for NavigationCompleted before marking as fully ready
}

void MainWindow::OnNavigationCompleted() {
    m_webview_ready = true;
    FlushLogCache();
    PostMessage(m_hwnd, WM_UIREADY, 0, 0);
}

void MainWindow::FlushLogCache() {
    if (!m_webview_ready || !m_webview) return;
    for (auto& log : m_log_cache) {
        json j;
        j["type"] = "log";
        j["level"] = log.level;
        j["message"] = log.message;
        std::wstring s = utf8_to_wide(j.dump());
        m_webview->PostWebMessageAsJson(s.c_str());
    }
    m_log_cache.clear();
}

void MainWindow::OnWebMessage(const std::string& msg) {
    if (m_command_handler) {
        m_command_handler(msg);
    }
}

void MainWindow::ResizeWebView() {
    if (!m_controller) return;
    RECT bounds;
    GetClientRect(m_hwnd, &bounds);
    m_controller->put_Bounds(bounds);
}

void MainWindow::PostLog(const std::string& level, const std::string& message) {
    UiLogData* data = new UiLogData();
    data->level = level;
    data->message = message;
    PostMessage(m_hwnd, WM_UILOG, (WPARAM)data, 0);
}

void MainWindow::PostStatus(const std::string& status, const std::string& text) {
    UiStatusData* data = new UiStatusData();
    data->status = status;
    data->text = text;
    PostMessage(m_hwnd, WM_UISTATUS, (WPARAM)data, 0);
}

void MainWindow::PostInfo(const std::string& id, const std::string& value) {
    UiInfoData* data = new UiInfoData();
    data->id = id;
    data->value = value;
    PostMessage(m_hwnd, WM_UIINFO, (WPARAM)data, 0);
}

void MainWindow::PostMessageEvent(bool is_group, const std::string& content) {
    UiMessageData* data = new UiMessageData();
    data->is_group = is_group;
    data->content = content;
    PostMessage(m_hwnd, WM_UIMESSAGE, (WPARAM)data, 0);
}

void MainWindow::DoPostLog(UiLogData* data) {
    if (!m_webview_ready || !m_webview) {
        m_log_cache.push_back(*data);
        return;
    }
    json j;
    j["type"] = "log";
    j["level"] = data->level;
    j["message"] = data->message;
    std::wstring s = utf8_to_wide(j.dump());
    m_webview->PostWebMessageAsJson(s.c_str());
}

void MainWindow::DoPostStatus(UiStatusData* data) {
    if (!m_webview_ready || !m_webview) return;
    json j;
    j["type"] = "status";
    j["status"] = data->status;
    j["text"] = data->text;
    std::wstring s = utf8_to_wide(j.dump());
    m_webview->PostWebMessageAsJson(s.c_str());
}

void MainWindow::DoPostInfo(UiInfoData* data) {
    if (!m_webview_ready || !m_webview) return;
    json j;
    j["type"] = "info";
    j["id"] = data->id;
    j["value"] = data->value;
    std::wstring s = utf8_to_wide(j.dump());
    m_webview->PostWebMessageAsJson(s.c_str());
}

void MainWindow::DoPostMessage(UiMessageData* data) {
    if (!m_webview_ready || !m_webview) return;
    json j;
    j["type"] = "message";
    j["isGroup"] = data->is_group;
    j["content"] = data->content;
    std::wstring s = utf8_to_wide(j.dump());
    m_webview->PostWebMessageAsJson(s.c_str());
}

std::wstring MainWindow::ExtractHtmlFromResource() {
    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(NULL, exePath, MAX_PATH);
    std::wstring dirPath = std::wstring(exePath);
    size_t pos = dirPath.find_last_of(L"\\");
    if (pos != std::wstring::npos) {
        dirPath = dirPath.substr(0, pos);
    }

    std::wstring uiDir = dirPath + L"\\ui";
    std::wstring htmlPath = uiDir + L"\\index.html";

    DWORD attr = GetFileAttributesW(uiDir.c_str());
    if (attr == INVALID_FILE_ATTRIBUTES) {
        CreateDirectoryW(uiDir.c_str(), NULL);
    }

    HRSRC hResource = FindResourceW(NULL, MAKEINTRESOURCEW(IDR_INDEX_HTML), RT_RCDATA);
    if (!hResource) {
        return L"";
    }

    HGLOBAL hGlobal = LoadResource(NULL, hResource);
    if (!hGlobal) {
        return L"";
    }

    DWORD size = SizeofResource(NULL, hResource);
    if (size == 0) {
        FreeResource(hGlobal);
        return L"";
    }

    const char* data = reinterpret_cast<const char*>(LockResource(hGlobal));
    if (!data) {
        FreeResource(hGlobal);
        return L"";
    }

    HANDLE hFile = CreateFileW(
        htmlPath.c_str(),
        GENERIC_WRITE,
        0,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (hFile == INVALID_HANDLE_VALUE) {
        UnlockResource(hGlobal);
        FreeResource(hGlobal);
        return L"";
    }

    DWORD bytesWritten = 0;
    BOOL result = WriteFile(hFile, data, size, &bytesWritten, NULL);

    CloseHandle(hFile);
    UnlockResource(hGlobal);
    FreeResource(hGlobal);

    if (result && bytesWritten == size) {
        return htmlPath;
    }
    return L"";
}
