#include "webview_handlers.h"
#include "json.hpp"
#include <string>
#include <cstdio>

using json = nlohmann::json;



static std::wstring utf8_to_wide(const std::string& utf8) {
    if (utf8.empty()) return L"";
    int len = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), (int)utf8.size(), NULL, 0);
    if (len <= 0) return L"";
    std::wstring result(len, 0);
    MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), (int)utf8.size(), &result[0], len);
    return result;
}

// CreateEnvironmentCompletedHandler
ULONG STDMETHODCALLTYPE CreateEnvironmentCompletedHandler::AddRef() {
    return InterlockedIncrement(&m_refCount);
}

ULONG STDMETHODCALLTYPE CreateEnvironmentCompletedHandler::Release() {
    ULONG ref = InterlockedDecrement(&m_refCount);
    if (ref == 0) delete this;
    return ref;
}

HRESULT STDMETHODCALLTYPE CreateEnvironmentCompletedHandler::QueryInterface(REFIID riid, LPVOID* ppvObj) {
    if (riid == IID_IUnknown || riid == IID_ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler) {
        *ppvObj = this;
        AddRef();
        return S_OK;
    }
    *ppvObj = NULL;
    return E_NOINTERFACE;
}

HRESULT STDMETHODCALLTYPE CreateEnvironmentCompletedHandler::Invoke(HRESULT errorCode, ICoreWebView2Environment* environment) {

    if (errorCode != S_OK) {
        wchar_t buf[256];
        swprintf_s(buf, 256, L"Environment creation failed: 0x%08X", errorCode);
        MessageBoxW(m_window->GetHwnd(), buf, L"Error", MB_ICONERROR);
        Release();
        return errorCode;
    }
    environment->CreateCoreWebView2Controller(m_window->GetHwnd(), new CreateControllerCompletedHandler(m_window));
    Release();
    return S_OK;
}

// CreateControllerCompletedHandler
ULONG STDMETHODCALLTYPE CreateControllerCompletedHandler::AddRef() {
    return InterlockedIncrement(&m_refCount);
}

ULONG STDMETHODCALLTYPE CreateControllerCompletedHandler::Release() {
    ULONG ref = InterlockedDecrement(&m_refCount);
    if (ref == 0) delete this;
    return ref;
}

HRESULT STDMETHODCALLTYPE CreateControllerCompletedHandler::QueryInterface(REFIID riid, LPVOID* ppvObj) {
    if (riid == IID_IUnknown || riid == IID_ICoreWebView2CreateCoreWebView2ControllerCompletedHandler) {
        *ppvObj = this;
        AddRef();
        return S_OK;
    }
    *ppvObj = NULL;
    return E_NOINTERFACE;
}

HRESULT STDMETHODCALLTYPE CreateControllerCompletedHandler::Invoke(HRESULT errorCode, ICoreWebView2Controller* controller) {

    if (errorCode != S_OK) {
        wchar_t buf[256];
        swprintf_s(buf, 256, L"Controller creation failed: 0x%08X", errorCode);
        MessageBoxW(m_window->GetHwnd(), buf, L"Error", MB_ICONERROR);
        Release();
        return errorCode;
    }

    ICoreWebView2* webview = nullptr;
    controller->get_CoreWebView2(&webview);
    controller->AddRef();
    if (webview) webview->AddRef();

    m_window->SetWebView(webview, controller);

    webview->add_WebMessageReceived(new WebMessageHandler(m_window), nullptr);
    webview->add_NavigationCompleted(new NavigationCompletedHandler(m_window), nullptr);

    RECT bounds;
    GetClientRect(m_window->GetHwnd(), &bounds);
    controller->put_Bounds(bounds);

    std::wstring html_file = MainWindow::ExtractHtmlFromResource();
    if (html_file.empty()) {
        wchar_t html_path_buf[MAX_PATH];
        GetModuleFileNameW(NULL, html_path_buf, MAX_PATH);
        std::wstring html_dir(html_path_buf);
        size_t pos = html_dir.find_last_of(L"\\/");
        if (pos != std::wstring::npos) {
            html_dir = html_dir.substr(0, pos);
        }
        html_file = html_dir + L"\\ui\\index.html";
    }
    std::wstring url = L"file:///" + html_file;
    for (auto& c : url) {
        if (c == L'\\') c = L'/';
    }
    webview->Navigate(url.c_str());

    Release();
    return S_OK;
}

// WebMessageHandler
ULONG STDMETHODCALLTYPE WebMessageHandler::AddRef() {
    return InterlockedIncrement(&m_refCount);
}

ULONG STDMETHODCALLTYPE WebMessageHandler::Release() {
    ULONG ref = InterlockedDecrement(&m_refCount);
    if (ref == 0) delete this;
    return ref;
}

HRESULT STDMETHODCALLTYPE WebMessageHandler::QueryInterface(REFIID riid, LPVOID* ppvObj) {
    if (riid == IID_IUnknown || riid == IID_ICoreWebView2WebMessageReceivedEventHandler) {
        *ppvObj = this;
        AddRef();
        return S_OK;
    }
    *ppvObj = NULL;
    return E_NOINTERFACE;
}

HRESULT STDMETHODCALLTYPE WebMessageHandler::Invoke(
    ICoreWebView2* sender,
    ICoreWebView2WebMessageReceivedEventArgs* args) {
    PWSTR message = nullptr;
    args->TryGetWebMessageAsString(&message);
    if (message) {
        std::wstring wmsg(message);
        int len = WideCharToMultiByte(CP_UTF8, 0, wmsg.c_str(), (int)wmsg.size(), NULL, 0, NULL, NULL);
        std::string msg(len, 0);
        WideCharToMultiByte(CP_UTF8, 0, wmsg.c_str(), (int)wmsg.size(), &msg[0], len, NULL, NULL);
        CoTaskMemFree(message);
        m_window->OnWebMessage(msg);
    }
    return S_OK;
}

// NavigationCompletedHandler
ULONG STDMETHODCALLTYPE NavigationCompletedHandler::AddRef() {
    return InterlockedIncrement(&m_refCount);
}

ULONG STDMETHODCALLTYPE NavigationCompletedHandler::Release() {
    ULONG ref = InterlockedDecrement(&m_refCount);
    if (ref == 0) delete this;
    return ref;
}

HRESULT STDMETHODCALLTYPE NavigationCompletedHandler::QueryInterface(REFIID riid, LPVOID* ppvObj) {
    if (riid == IID_IUnknown || riid == IID_ICoreWebView2NavigationCompletedEventHandler) {
        *ppvObj = this;
        AddRef();
        return S_OK;
    }
    *ppvObj = NULL;
    return E_NOINTERFACE;
}

HRESULT STDMETHODCALLTYPE NavigationCompletedHandler::Invoke(
    ICoreWebView2* sender,
    ICoreWebView2NavigationCompletedEventArgs* args) {
    BOOL is_success = FALSE;
    args->get_IsSuccess(&is_success);
    if (is_success) {
        m_window->OnNavigationCompleted();
    } else {
        MessageBoxW(m_window->GetHwnd(), L"Navigation failed", L"Error", MB_ICONERROR);
    }
    return S_OK;
}
