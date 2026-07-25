#pragma once

#include <windows.h>
#include <string>
#include <functional>
#include <WebView2.h>
#include <vector>

#define WM_UILOG (WM_USER + 100)
#define WM_UISTATUS (WM_USER + 101)
#define WM_UIINFO (WM_USER + 102)
#define WM_UIMESSAGE (WM_USER + 103)
#define WM_UIREADY (WM_USER + 104)

struct UiLogData {
    std::string level;
    std::string message;
};

struct UiStatusData {
    std::string status;
    std::string text;
};

struct UiInfoData {
    std::string id;
    std::string value;
};

struct UiMessageData {
    bool is_group;
    std::string content;
};

class MainWindow {
public:
    MainWindow();
    ~MainWindow();

    bool Create(HINSTANCE hInstance, int nCmdShow);
    void Show();
    void RunMessageLoop();

    void PostLog(const std::string& level, const std::string& message);
    void PostStatus(const std::string& status, const std::string& text);
    void PostInfo(const std::string& id, const std::string& value);
    void PostMessageEvent(bool is_group, const std::string& content);

    void SetCommandHandler(std::function<void(const std::string&)> handler) {
        m_command_handler = std::move(handler);
    }

    void SetReadyHandler(std::function<void()> handler) {
        m_ready_handler = std::move(handler);
    }

    HWND GetHwnd() const { return m_hwnd; }
    void SetWebView(ICoreWebView2* webview, ICoreWebView2Controller* controller);
    void OnReady();
    void OnWebMessage(const std::string& msg);
    void OnNavigationCompleted();

    static std::wstring ExtractHtmlFromResource();

private:
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);

    void InitializeWebView();
    void ResizeWebView();
    void FlushLogCache();

    void DoPostLog(UiLogData* data);
    void DoPostStatus(UiStatusData* data);
    void DoPostInfo(UiInfoData* data);
    void DoPostMessage(UiMessageData* data);

    HWND m_hwnd;
    ICoreWebView2* m_webview;
    ICoreWebView2Controller* m_controller;
    std::function<void(const std::string&)> m_command_handler;
    std::function<void()> m_ready_handler;
    bool m_webview_ready;
    std::vector<UiLogData> m_log_cache;
};
