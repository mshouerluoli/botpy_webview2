#pragma once

#include <windows.h>
#include <string>
#include <vector>
#include <functional>
#include <mutex>
#include <atomic>
#include "resource.h"

// Forward declaration for PluginInstance
struct PluginInstance;

// Thread-safe message structures
struct UILogMsg {
    std::string level;
    std::string msg;
};

struct UIStatusMsg {
    std::string status;
    std::string text;
};

struct UIInfoMsg {
    std::string id;
    std::string value;
};

struct UIMessageEventMsg  
{
    std::string id;
    std::string content;
    std::string username;
    std::string sender_id;
    std::string channel_id;
    bool is_group;
    bool is_groupat;
    std::string openid;
    std::string group_openid;
};

struct UIPluginsMsg {
    std::string json_str;
};

struct ThemeColors {
    COLORREF bgBase;          // #0f1222
    COLORREF bgDeep;          // #0a0d1a
    COLORREF bgCard;          // rgba(28,34,56,0.72) -> #1c2238
    COLORREF bgElevated;      // rgba(45,53,88,0.6) -> #2d3558
    COLORREF borderSoft;      // rgba(120,140,200,0.12)
    COLORREF borderStrong;    // rgba(120,140,200,0.28)
    COLORREF accent;          // #7c83ff
    COLORREF accent2;         // #a78bfa
    COLORREF textPrimary;     // #e8ecf5
    COLORREF textSecondary;   // #a0aac4
    COLORREF textMuted;       // #6b7394
    COLORREF green;           // #4ade80
    COLORREF greenBg;         // rgba(74,222,128,0.12)
    COLORREF red;             // #f87171
    COLORREF redBg;           // rgba(248,113,113,0.14)
    COLORREF yellow;          // #fbbf24
    COLORREF yellowBg;        // rgba(251,191,36,0.14)
    COLORREF logInfo;         // #93c5fd
    COLORREF logSuccess;      // #86efac
    COLORREF logError;        // #fca5a5
    COLORREF logWarning;      // #fcd34d
    COLORREF logMessage;      // #c4b5fd
};

// Plugin management function typedefs
typedef std::function<std::vector<PluginInstance>()> GetPluginsFunc;
typedef std::function<bool(const std::string&, bool)> TogglePluginFunc;
typedef std::function<bool(const std::string&)> UnloadPluginFunc;
typedef std::function<bool(const std::string&)> ReloadPluginsFunc;

// 插件项布局结构（包含按钮区域）
struct PluginItemLayout {
    RECT itemRect;          // 整个插件项矩形
    RECT toggleBtnRect;     // 启用/停用按钮区域
    RECT unloadBtnRect;     // 卸载按钮区域
};

class MainWindow {
public:
    MainWindow();
    ~MainWindow();

    bool Create(HINSTANCE hInst, int nCmdShow);
    void Show(int nCmdShow);

    // UI 更新函数 (called from UI thread only)
    void SetConnected(bool connected);
    void SetAppId(const std::wstring& appId);
    void SetNickname(const std::wstring& nickname);
    void SetSessionId(const std::wstring& sessionId);
    void SetMessageCount(int count);
    void SetHeartbeatCount(int count);
    void LimitLogLines(int maxLines);
    void AppendLog(const std::wstring& message, const std::wstring& type = L"info");

    // 机器人回调接口 (called from background thread, thread-safe)
    void PostLog(const std::string& level, const std::string& msg);
    void PostLogUtf8(const std::string& level, const std::string& msg);
    void PostStatus(const std::string& status, const std::string& text);
    void PostInfo(const std::string& id, const std::string& value);
    void PostMessageEvent(bool is_group, const std::string& username, const std::string& content);
    void PostPlugins(const std::string& json_str);

    // 命令处理器
    void SetCommandHandler(std::function<void(const std::string&)> handler);
    void SetReadyHandler(std::function<void()> handler);

    // 插件管理回调接口
    void SetPluginGetHandler(GetPluginsFunc func);
    void SetPluginToggleHandler(TogglePluginFunc func);
    void SetPluginUnloadHandler(UnloadPluginFunc func);
    void SetPluginReloadHandler(ReloadPluginsFunc func);
    void RefreshPluginList();

    // 运行消息循环
    void RunMessageLoop();

    HWND GetHwnd() const { return m_hwnd; }

private:
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);

    void OnCreate(HWND hwnd);
    void OnPaint();
    void OnSize(int cx, int cy);
    void OnCommand(WPARAM wParam);
    void OnDestroy();
    void OnTabSelChange();
    void OnLButtonDown(int x, int y);

    void CreateChildControls(HWND parent);
    void CreateLogView(HWND parent);
    void CreatePluginView(HWND parent);

    // 字符串转换辅助
    static std::wstring StringToWString(const std::string& str);
    static std::wstring Utf8ToWString(const std::string& str);

    // Drawing helpers
    void DrawRoundRect(HDC hdc, int x, int y, int w, int h, int radius, COLORREF fill, COLORREF border);
    void DrawRoundRect(HDC hdc, const RECT& rect, int radius, COLORREF fill, COLORREF border);
    void DrawGradientRect(HDC hdc, int x, int y, int w, int h, COLORREF color1, COLORREF color2, bool vertical = true);
    
    void DrawBackground(HDC hdc);
    void DrawSidebar(HDC hdc, const RECT& rect);
    void DrawContent(HDC hdc, const RECT& rect);
    void DrawStatusBar(HDC hdc, const RECT& rect);
    void DrawInfoItem(HDC hdc, const RECT& rect, const std::wstring& label, const std::wstring& value);
    void DrawTabBar(HDC hdc, const RECT& rect);
    void DrawConnectionDot(HDC hdc, int cx, int cy, int radius, bool connected);
    void DrawPluginPanel(HDC hdc, const RECT& rect);
    void DrawPluginItem(HDC hdc, const PluginItemLayout& layout, const PluginInstance& plugin, int index);
    void DrawPluginToolbar(HDC hdc, const RECT& rect);

    void CalculateLayout();
    void CalculatePluginLayout();

    // Thread-safe UI message handlers (called from UI thread via PostMessage)
    void HandleUILogMsg(UILogMsg* msg);
    void HandleUIStatusMsg(UIStatusMsg* msg);
    void HandleUIInfoMsg(UIInfoMsg* msg);
    void HandleUIMessageEventMsg(UIMessageEventMsg* msg);
    void HandleUIPluginsMsg(UIPluginsMsg* msg);

    HWND m_hwnd;
    HINSTANCE m_hInstance;
    HMODULE m_hRichEditDll;
    std::atomic<bool> m_windowDestroyed;  // 防止窗口销毁后发送消息

    HWND m_hStatusText;
    HWND m_hAppIdValue;
    HWND m_hNicknameValue;
    HWND m_hSessionIdValue;
    HWND m_hMsgCountValue;
    HWND m_hHeartbeatValue;
    HWND m_hTabControl;
    HWND m_hLogEdit;
    HWND m_hPluginRefreshBtn;

    RECT m_sidebarRect;
    RECT m_contentRect;
    RECT m_statusBarRect;
    std::vector<RECT> m_infoItemRects;
    RECT m_tabBarRect;
    RECT m_pluginToolbarRect;
    RECT m_pluginListAreaRect;
    std::vector<PluginItemLayout> m_pluginItemLayouts;

    bool m_isConnected;
    std::wstring m_appId;
    std::wstring m_nickname;
    std::wstring m_sessionId;
    int m_messageCount;
    int m_heartbeatCount;
    int m_activeTab;
    int m_selectedPluginIndex;
    int m_hoverPluginIndex;        // 鼠标悬停的插件项索引
    int m_hoverPluginButton;       // 鼠标悬停的按钮 (0=无, 1=toggle, 2=unload)

    // 插件数据
    std::vector<PluginInstance> m_plugins;

    // 机器人命令回调
    std::function<void(const std::string&)> m_commandHandler;
    std::function<void()> m_readyHandler;

    // 插件管理回调
    GetPluginsFunc m_getPluginsFunc;
    TogglePluginFunc m_togglePluginFunc;
    UnloadPluginFunc m_unloadPluginFunc;
    ReloadPluginsFunc m_reloadPluginsFunc;

    ThemeColors m_colors;
    HFONT m_hFont;
    HFONT m_hFontBold;
    HFONT m_hFontLabel;
    HFONT m_hFontMono;
    HFONT m_hFontTitle;
    HFONT m_hFontSmall;
    HFONT m_hPluginNameFont;   // 插件名称字体（粗体）
    HFONT m_hPluginMetaFont;   // 插件元数据字体
    HFONT m_hPluginTagFont;    // 标签字体
    HFONT m_hPluginBtnFont;    // 按钮字体
};
