#include "MainWindow.h"
#include "Miao.h"
#include <commctrl.h>
#include <richedit.h>
#include <ctime>

#ifndef SC_ALL
#define SC_ALL 2
#endif
#ifndef SC_SELECTION
#define SC_SELECTION 1
#endif

#pragma comment(lib, "comctl32.lib")

// Helper to create UI colors matching the HTML theme
ThemeColors GetDefaultThemeColors() {
    ThemeColors c;
    c.bgBase = RGB(0x0f, 0x12, 0x22);
    c.bgDeep = RGB(0x0a, 0x0d, 0x1a);
    c.bgCard = RGB(0x1c, 0x22, 0x38);
    c.bgElevated = RGB(0x2d, 0x35, 0x58);
    c.borderSoft = RGB(0x2d, 0x35, 0x58);  // Simplified
    c.borderStrong = RGB(0x50, 0x5c, 0x8a); // Simplified
    c.accent = RGB(0x7c, 0x83, 0xff);
    c.accent2 = RGB(0xa7, 0x8b, 0xfa);
    c.textPrimary = RGB(0xe8, 0xec, 0xf5);
    c.textSecondary = RGB(0xa0, 0xaa, 0xc4);
    c.textMuted = RGB(0x6b, 0x73, 0x94);
    c.green = RGB(0x4a, 0xde, 0x80);
    c.greenBg = RGB(0x15, 0x35, 0x23); // Approximate
    c.red = RGB(0xf8, 0x71, 0x71);
    c.redBg = RGB(0x3d, 0x18, 0x18);   // Approximate
    c.yellow = RGB(0xfb, 0xbf, 0x24);
    c.yellowBg = RGB(0x3d, 0x33, 0x15); // Approximate
    c.logInfo = RGB(0x93, 0xc5, 0xfd);
    c.logSuccess = RGB(0x86, 0xef, 0xac);
    c.logError = RGB(0xfc, 0xa5, 0xa5);
    c.logWarning = RGB(0xfc, 0xd3, 0x4d);
    c.logMessage = RGB(0xc4, 0xb5, 0xfd);
    return c;
}

// Helper: create a brush from COLORREF
HBRUSH CreateBrush(COLORREF color) {
    return CreateSolidBrush(color);
}

// Helper: create a pen from COLORREF
HPEN CreatePen(int style, int width, COLORREF color) {
    return ::CreatePen(style, width, color);
}

MainWindow::MainWindow() 
    : m_hwnd(nullptr)
    , m_hInstance(nullptr)
    , m_hRichEditDll(nullptr)
    , m_windowDestroyed(false)
    , m_hStatusText(nullptr)
    , m_hAppIdValue(nullptr)
    , m_hNicknameValue(nullptr)
    , m_hSessionIdValue(nullptr)
    , m_hMsgCountValue(nullptr)
    , m_hHeartbeatValue(nullptr)
    , m_hTabControl(nullptr)
    , m_hLogEdit(nullptr)
    , m_hPluginRefreshBtn(nullptr)
    , m_isConnected(false)
    , m_appId(L"-")
    , m_nickname(L"-")
    , m_sessionId(L"-")
    , m_messageCount(0)
    , m_heartbeatCount(0)
    , m_activeTab(0)
    , m_selectedPluginIndex(-1)
    , m_hoverPluginIndex(-1)
    , m_hoverPluginButton(0)
    , m_colors(GetDefaultThemeColors())
    , m_hFont(nullptr)
    , m_hFontBold(nullptr)
    , m_hFontLabel(nullptr)
    , m_hFontMono(nullptr)
    , m_hFontTitle(nullptr)
    , m_hFontSmall(nullptr)
    , m_hPluginNameFont(nullptr)
    , m_hPluginMetaFont(nullptr)
    , m_hPluginTagFont(nullptr)
    , m_hPluginBtnFont(nullptr) {
}

MainWindow::~MainWindow() {
    if (m_hFont) DeleteObject(m_hFont);
    if (m_hFontBold) DeleteObject(m_hFontBold);
    if (m_hFontLabel) DeleteObject(m_hFontLabel);
    if (m_hFontMono) DeleteObject(m_hFontMono);
    if (m_hFontTitle) DeleteObject(m_hFontTitle);
    if (m_hFontSmall) DeleteObject(m_hFontSmall);
    if (m_hPluginNameFont) DeleteObject(m_hPluginNameFont);
    if (m_hPluginMetaFont) DeleteObject(m_hPluginMetaFont);
    if (m_hPluginTagFont) DeleteObject(m_hPluginTagFont);
    if (m_hPluginBtnFont) DeleteObject(m_hPluginBtnFont);
}

bool MainWindow::Create(HINSTANCE hInst, int nCmdShow) {
    m_hInstance = hInst;

    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = MainWindow::WindowProc;
    wc.hInstance = hInst;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = L"MiaoBotWindowClass";
    wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    wc.hIconSm = LoadIcon(nullptr, IDI_APPLICATION);

    if (!RegisterClassExW(&wc)) {
        return false;
    }

    m_hwnd = CreateWindowExW(
        0, L"MiaoBotWindowClass", L"Miao Bot - QQ Robot Console",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 900, 560,
        nullptr, nullptr, hInst, this
    );

    if (!m_hwnd) {
        return false;
    }

    // Create fonts matching HTML design
    m_hFont = CreateFontW(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                          DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                          DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    m_hFontBold = CreateFontW(14, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                              DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                              DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    m_hFontLabel = CreateFontW(11, 0, 0, 0, FW_MEDIUM, FALSE, FALSE, FALSE,
                              DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                              DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    m_hFontMono = CreateFontW(13, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                              DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                              DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Consolas");
    m_hFontTitle = CreateFontW(16, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                               DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                               DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    m_hFontSmall = CreateFontW(10, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                               DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                               DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    // 插件管理专用字体（更大更清晰）
    m_hPluginNameFont = CreateFontW(18, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                                    DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                    DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    m_hPluginMetaFont = CreateFontW(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                    DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                    DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    m_hPluginTagFont = CreateFontW(12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                   DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                   DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    m_hPluginBtnFont = CreateFontW(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                   DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                   DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");

    Show(nCmdShow);
    return true;
}

void MainWindow::Show(int nCmdShow) {
    ShowWindow(m_hwnd, nCmdShow);
    UpdateWindow(m_hwnd);
}

LRESULT CALLBACK MainWindow::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    MainWindow* pThis = nullptr;

    if (uMsg == WM_NCCREATE) {
        CREATESTRUCTW* pCreate = (CREATESTRUCTW*)lParam;
        pThis = (MainWindow*)pCreate->lpCreateParams;
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)pThis);
        pThis->m_hwnd = hwnd;
    } else {
        pThis = (MainWindow*)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
    }

    if (pThis) {
        return pThis->HandleMessage(uMsg, wParam, lParam);
    }

    return DefWindowProcW(hwnd, uMsg, wParam, lParam);
}

LRESULT MainWindow::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CREATE:
            OnCreate(m_hwnd);
            return 0;
        case WM_USER + 1:
            // 延迟添加日志
            AppendLog(L"Miao Bot 控制台已就绪", L"info");
            AppendLog(L"正在初始化...", L"info");
            return 0;
        case WM_USER + 2:
            // 延迟初始化数据
            SetAppId(L"123456789");
            SetNickname(L"Miao Bot");
            SetSessionId(L"");
            SetMessageCount(0);
            SetHeartbeatCount(0);
            SetConnected(false);
            
            // 触发就绪事件（启动机器人）
            if (m_readyHandler) {
                m_readyHandler();
            }
            return 0;
        case WM_UI_LOG:
            HandleUILogMsg((UILogMsg*)lParam);
            return 0;
        case WM_UI_STATUS:
            HandleUIStatusMsg((UIStatusMsg*)lParam);
            return 0;
        case WM_UI_INFO:
            HandleUIInfoMsg((UIInfoMsg*)lParam);
            return 0;
        case WM_UI_MESSAGE_EVENT:
            HandleUIMessageEventMsg((UIMessageEventMsg*)lParam);
            return 0;
        case WM_UI_PLUGINS_UPDATE:
            HandleUIPluginsMsg((UIPluginsMsg*)lParam);
            return 0;
        case WM_PAINT:
            OnPaint();
            return 0;
        case WM_SIZE:
            OnSize(LOWORD(lParam), HIWORD(lParam));
            return 0;
        case WM_COMMAND:
            OnCommand(wParam);
            return 0;
        case WM_LBUTTONDOWN:
            OnLButtonDown(LOWORD(lParam), HIWORD(lParam));
            return 0;
        case WM_MOUSEMOVE: {
            int x = LOWORD(lParam);
            int y = HIWORD(lParam);
            POINT pt = {x, y};
            
            // 仅在插件管理选项卡时处理悬停
            if (m_activeTab == 1) {
                int newHoverIndex = -1;
                int newHoverButton = 0;
                
                for (size_t i = 0; i < m_pluginItemLayouts.size() && i < m_plugins.size(); i++) {
                    const PluginItemLayout& layout = m_pluginItemLayouts[i];
                    if (PtInRect(&layout.toggleBtnRect, pt)) {
                        newHoverIndex = (int)i;
                        newHoverButton = 1;
                        break;
                    }
                    if (PtInRect(&layout.unloadBtnRect, pt)) {
                        newHoverIndex = (int)i;
                        newHoverButton = 2;
                        break;
                    }
                    if (PtInRect(&layout.itemRect, pt)) {
                        newHoverIndex = (int)i;
                        newHoverButton = 0;
                        break;
                    }
                }
                
                if (newHoverIndex != m_hoverPluginIndex || newHoverButton != m_hoverPluginButton) {
                    m_hoverPluginIndex = newHoverIndex;
                    m_hoverPluginButton = newHoverButton;
                    InvalidateRect(m_hwnd, NULL, TRUE);
                }
                
                // 跟踪鼠标离开
                TRACKMOUSEEVENT tme = {};
                tme.cbSize = sizeof(tme);
                tme.dwFlags = TME_LEAVE;
                tme.hwndTrack = m_hwnd;
                TrackMouseEvent(&tme);
            }
            return 0;
        }
        case WM_MOUSELEAVE:
            if (m_hoverPluginIndex != -1 || m_hoverPluginButton != 0) {
                m_hoverPluginIndex = -1;
                m_hoverPluginButton = 0;
                InvalidateRect(m_hwnd, NULL, TRUE);
            }
            return 0;
        case WM_DRAWITEM: {
            LPDRAWITEMSTRUCT pdis = (LPDRAWITEMSTRUCT)lParam;
            if (pdis->CtlType == ODT_BUTTON && pdis->CtlID == IDC_PLUGIN_REFRESH) {
                HDC hdc = pdis->hDC;
                RECT rect = pdis->rcItem;
                bool selected = (pdis->itemState & ODS_SELECTED) != 0;
                
                COLORREF bgColor = selected ? m_colors.accent : RGB(0x2a, 0x2f, 0x4d);
                COLORREF borderColor = selected ? m_colors.accent : m_colors.accent;
                COLORREF textColor = selected ? RGB(0xFF, 0xFF, 0xFF) : RGB(0xc7, 0xcb, 0xff);
                
                HBRUSH hBgBrush = CreateBrush(bgColor);
                FillRect(hdc, &rect, hBgBrush);
                DeleteObject(hBgBrush);
                
                HBRUSH hBorderBrush = CreateBrush(borderColor);
                FrameRect(hdc, &rect, hBorderBrush);
                DeleteObject(hBorderBrush);
                
                const wchar_t* btnText = L"刷新插件";
                SetTextColor(hdc, textColor);
                SetBkMode(hdc, TRANSPARENT);
                SelectObject(hdc, m_hFontSmall);
                int textLen = lstrlenW(btnText);
                int textX = rect.left + (rect.right - rect.left - textLen * 9) / 2;
                int textY = rect.top + (rect.bottom - rect.top - 12) / 2;
                TextOutW(hdc, textX, textY, btnText, textLen);
            }
            return 0;
        }
        case WM_NOTIFY: {
            LPNMHDR pnmh = (LPNMHDR)lParam;
            if (pnmh->idFrom == IDC_TAB_CONTROL && pnmh->code == TCN_SELCHANGE) {
                OnTabSelChange();
            }
            break;
        }
        case WM_CLOSE:
            DestroyWindow(m_hwnd);
            return 0;
        case WM_DESTROY:
            OnDestroy();
            PostQuitMessage(0);
            return 0;
    }

    return DefWindowProcW(m_hwnd, uMsg, wParam, lParam);
}

void MainWindow::OnCreate(HWND hwnd) {
    INITCOMMONCONTROLSEX icc = {};
    icc.dwSize = sizeof(icc);
    icc.dwICC = ICC_TAB_CLASSES;
    InitCommonControlsEx(&icc);

    CalculateLayout();
    CreateChildControls(hwnd);

    // 初始化控件位置
    RECT clientRect;
    GetClientRect(hwnd, &clientRect);
    int width = clientRect.right - clientRect.left;
    int height = clientRect.bottom - clientRect.top;
    OnSize(width, height);

    // 使用 PostMessage 延迟添加日志
    PostMessageW(hwnd, WM_USER + 1, 0, 0);
}

void MainWindow::CalculateLayout() {
    RECT clientRect;
    GetClientRect(m_hwnd, &clientRect);

    int width = clientRect.right - clientRect.left;
    int height = clientRect.bottom - clientRect.top;

    // Main padding: 12px
    // Sidebar width: 256px, gap: 12px
    const int PADDING = 12;
    const int GAP = 12;
    const int SIDEBAR_W = 256;

    m_sidebarRect = {PADDING, PADDING, PADDING + SIDEBAR_W, height - PADDING};
    m_contentRect = {
        PADDING + SIDEBAR_W + GAP,
        PADDING,
        width - PADDING,
        height - PADDING
    };

    // Status bar inside sidebar
    int sidebarInnerLeft = m_sidebarRect.left + 16;
    int sidebarInnerRight = m_sidebarRect.right - 16;
    int yPos = m_sidebarRect.top + 50; // h2 title area

    m_statusBarRect = {
        sidebarInnerLeft, yPos,
        sidebarInnerRight, yPos + 40
    };
    yPos += 40 + 10; // gap

    // Info items (会话ID需要更高的卡片以支持换行)
    m_infoItemRects.clear();
    // 每个卡片的高度：索引2是会话ID，使用76px（其他使用52px）
    int itemHeights[] = {52, 52, 76, 52, 52};
    for (int i = 0; i < 5; i++) {
        RECT itemRect = {
            sidebarInnerLeft, yPos,
            sidebarInnerRight, yPos + itemHeights[i]
        };
        m_infoItemRects.push_back(itemRect);
        yPos += itemHeights[i] + 10;
    }

    // Tab bar inside content area
    int contentInnerLeft = m_contentRect.left;
    int contentInnerRight = m_contentRect.right;
    m_tabBarRect = {
        contentInnerLeft, m_contentRect.top,
        contentInnerRight, m_contentRect.top + 48
    };

    // Plugin layout
    CalculatePluginLayout();
}

void MainWindow::CalculatePluginLayout() {
    // Toolbar area below tab bar
    m_pluginToolbarRect = {
        m_contentRect.left + 1,
        m_tabBarRect.bottom + 1,
        m_contentRect.right - 1,
        m_tabBarRect.bottom + 1 + 44
    };

    // Plugin list area
    m_pluginListAreaRect = {
        m_contentRect.left + 1,
        m_pluginToolbarRect.bottom + 1,
        m_contentRect.right - 1,
        m_contentRect.bottom - 1
    };

    // Calculate plugin item layouts (matching HTML design)
    m_pluginItemLayouts.clear();
    int itemHeight = 96;       // 增大高度以容纳更大字体
    int yPos = m_pluginListAreaRect.top + 8;
    int left = m_pluginListAreaRect.left + 8;
    int right = m_pluginListAreaRect.right - 8;

    int btnWidth = 80;         // 增大按钮宽度
    int btnHeight = 36;        // 增大按钮高度
    int btnGap = 10;

    for (size_t i = 0; i < m_plugins.size(); i++) {
        PluginItemLayout layout;
        layout.itemRect = {left, yPos, right, yPos + itemHeight};
        
        // 按钮区域在右侧
        int btnY = yPos + (itemHeight - btnHeight) / 2;
        layout.unloadBtnRect = {
            right - btnWidth - 16, btnY,
            right - 16, btnY + btnHeight
        };
        layout.toggleBtnRect = {
            layout.unloadBtnRect.left - btnWidth - btnGap, btnY,
            layout.unloadBtnRect.left - btnGap, btnY + btnHeight
        };
        
        m_pluginItemLayouts.push_back(layout);
        yPos += itemHeight + 12; // 12px gap between items
    }
}

void MainWindow::OnSize(int cx, int cy) {
    CalculateLayout();

    // 调整 Tab 控件大小和位置（覆盖选项卡区域）
    if (m_hTabControl && m_tabBarRect.right > m_tabBarRect.left &&
        m_tabBarRect.bottom > m_tabBarRect.top) {
        MoveWindow(m_hTabControl, m_tabBarRect.left, m_tabBarRect.top,
                   m_tabBarRect.right - m_tabBarRect.left,
                   m_tabBarRect.bottom - m_tabBarRect.top, TRUE);
    }

    // 验证矩形有效性后再移动 Edit 控件
    if (m_hLogEdit && 
        m_contentRect.right > m_contentRect.left && 
        m_contentRect.bottom > m_contentRect.top &&
        m_tabBarRect.bottom > m_tabBarRect.top) {
        int logLeft = m_contentRect.left + 1;
        int logTop = m_tabBarRect.bottom + 1;
        int logRight = m_contentRect.right - 1;
        int logBottom = m_contentRect.bottom - 1;
        if (logRight > logLeft && logBottom > logTop) {
            MoveWindow(m_hLogEdit, logLeft, logTop, 
                       logRight - logLeft, logBottom - logTop, TRUE);
        }
    }

    // 调整插件面板刷新按钮
    if (m_hPluginRefreshBtn && m_pluginToolbarRect.right > m_pluginToolbarRect.left) {
        int btnWidth = 90;
        int btnHeight = 32;
        int btnY = m_pluginToolbarRect.top + (m_pluginToolbarRect.bottom - m_pluginToolbarRect.top - btnHeight) / 2;
        
        MoveWindow(m_hPluginRefreshBtn, 
                   m_pluginToolbarRect.left + 10, btnY,
                   btnWidth, btnHeight, TRUE);
    }

    // 显示 Tab 控件（但禁用它，让我们自绘的选项卡接受点击）
    if (m_hTabControl) {
        ShowWindow(m_hTabControl, SW_SHOW);
    }

    // 根据当前选项卡显示/隐藏对应控件
    if (m_activeTab == 0) {
        if (m_hLogEdit) ShowWindow(m_hLogEdit, SW_SHOW);
        if (m_hPluginRefreshBtn) ShowWindow(m_hPluginRefreshBtn, SW_HIDE);
    } else {
        if (m_hLogEdit) ShowWindow(m_hLogEdit, SW_HIDE);
        if (m_hPluginRefreshBtn) ShowWindow(m_hPluginRefreshBtn, SW_SHOW);
    }

    InvalidateRect(m_hwnd, NULL, TRUE);
}

void MainWindow::CreateChildControls(HWND parent) {
    // 创建 Tab 控件（启用，用于处理点击事件）
    m_hTabControl = CreateWindowExW(
        0,  // 不使用 WS_EX_TRANSPARENT
        WC_TABCONTROLW, L"",
        WS_CHILD,
        0, 0, 1, 1,
        parent, (HMENU)IDC_TAB_CONTROL, m_hInstance, nullptr
    );

    if (m_hTabControl) {
        TCITEMW tie = {};
        tie.mask = TCIF_TEXT;
        wchar_t tab1[] = L"运行日志";
        tie.pszText = tab1;
        tie.cchTextMax = 8;
        TabCtrl_InsertItem(m_hTabControl, 0, &tie);
        wchar_t tab2[] = L"插件管理";
        tie.pszText = tab2;
        TabCtrl_InsertItem(m_hTabControl, 1, &tie);
    }

    // 创建日志编辑控件
    CreateLogView(parent);

    // 创建插件管理控件
    CreatePluginView(parent);
}

void MainWindow::CreatePluginView(HWND parent) {
    // 创建刷新按钮（工具栏）
    m_hPluginRefreshBtn = CreateWindowExW(
        0, L"BUTTON", L"刷新插件",
        WS_CHILD | BS_PUSHBUTTON | BS_OWNERDRAW,
        0, 0, 1, 1,
        parent, (HMENU)IDC_PLUGIN_REFRESH, m_hInstance, nullptr
    );

    // 初始隐藏刷新按钮
    if (m_hPluginRefreshBtn) ShowWindow(m_hPluginRefreshBtn, SW_HIDE);
}

void MainWindow::CreateLogView(HWND parent) {
    m_hLogEdit = CreateWindowExW(
        0, L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL,
        0, 0, 100, 100,
        parent, (HMENU)IDC_LOG_EDIT, m_hInstance, nullptr
    );

    if (m_hLogEdit && m_hFontMono) {
        SendMessage(m_hLogEdit, WM_SETFONT, (WPARAM)m_hFontMono, TRUE);
    }
}

void MainWindow::DrawRoundRect(HDC hdc, int x, int y, int w, int h, int radius, COLORREF fill, COLORREF border) {
    RECT r = {x, y, x + w, y + h};
    DrawRoundRect(hdc, r, radius, fill, border);
}

void MainWindow::DrawRoundRect(HDC hdc, const RECT& rect, int radius, COLORREF fill, COLORREF border) {
    int w = rect.right - rect.left;
    int h = rect.bottom - rect.top;

    if (w <= 1 || h <= 1) return;

    // 使用简单的矩形填充代替圆角矩形
    // 先画边框
    RECT borderRect = rect;
    HBRUSH hBorderBrush = CreateBrush(border);
    FillRect(hdc, &borderRect, hBorderBrush);
    DeleteObject(hBorderBrush);

    // 再画填充（缩小1像素）
    RECT innerRect = rect;
    InflateRect(&innerRect, -1, -1);
    if (innerRect.right > innerRect.left && innerRect.bottom > innerRect.top) {
        HBRUSH hFillBrush = CreateBrush(fill);
        FillRect(hdc, &innerRect, hFillBrush);
        DeleteObject(hFillBrush);
    }
}

void MainWindow::DrawBackground(HDC hdc) {
    RECT clientRect;
    GetClientRect(m_hwnd, &clientRect);

    // Fill with base color
    HBRUSH hBrush = CreateBrush(m_colors.bgBase);
    FillRect(hdc, &clientRect, hBrush);
    DeleteObject(hBrush);

    // Draw radial gradient approximation with solid fills
    // Top-left glow (approximate radial-gradient)
    RECT topGlow = {0, 0, clientRect.right / 2, clientRect.bottom / 3};
    // Use slightly lighter color for glow
    COLORREF glowColor = RGB(0x1a, 0x1f, 0x3a);
    HBRUSH hGlowBrush = CreateBrush(glowColor);
    FillRect(hdc, &topGlow, hGlowBrush);
    DeleteObject(hGlowBrush);
}

void MainWindow::DrawSidebar(HDC hdc, const RECT& rect) {
    // Draw sidebar card
    DrawRoundRect(hdc, rect, 16, m_colors.bgCard, m_colors.borderSoft);

    // Draw accent line before title (先画竖线，再画文字)
    int titleY = rect.top + 18;
    RECT accentLine = {rect.left + 16, titleY + 4, rect.left + 20, titleY + 20};
    HBRUSH hAccentBrush = CreateBrush(m_colors.accent);
    FillRect(hdc, &accentLine, hAccentBrush);
    DeleteObject(hAccentBrush);

    // Draw "机器人信息" title
    SetTextColor(hdc, m_colors.textPrimary);
    SetBkMode(hdc, TRANSPARENT);
    SelectObject(hdc, m_hFontTitle);
    TextOutW(hdc, rect.left + 26, titleY, L"机器人信息", 6);  // 文字右移，避开竖线
}

void MainWindow::DrawContent(HDC hdc, const RECT& rect) {
    DrawRoundRect(hdc, rect, 16, m_colors.bgCard, m_colors.borderSoft);
}

void MainWindow::DrawStatusBar(HDC hdc, const RECT& rect) {
    // Status bar background
    DrawRoundRect(hdc, rect, 8, m_colors.bgElevated, m_colors.borderStrong);

    // Draw dot
    int dotX = rect.left + 14;
    int dotY = (rect.top + rect.bottom) / 2;
    DrawConnectionDot(hdc, dotX, dotY, 5, m_isConnected);

    // Draw status text
    const wchar_t* statusText = m_isConnected ? L"已连接" : L"未连接";
    COLORREF textColor = m_isConnected ? m_colors.green : m_colors.textPrimary;

    SetTextColor(hdc, textColor);
    SetBkMode(hdc, TRANSPARENT);
    SelectObject(hdc, m_hFont);
    TextOutW(hdc, dotX + 14, rect.top + 10, statusText, lstrlenW(statusText));
}

void MainWindow::DrawConnectionDot(HDC hdc, int cx, int cy, int radius, bool connected) {
    COLORREF color = connected ? m_colors.green : m_colors.red;
    
    HBRUSH hBrush = CreateBrush(color);
    HGDIOBJ hOldBrush = SelectObject(hdc, hBrush);
    
    // 设置画笔为 NULL，这样只画填充
    HPEN hOldPen = (HPEN)SelectObject(hdc, GetStockObject(NULL_PEN));
    
    Ellipse(hdc, cx - radius, cy - radius, cx + radius, cy + radius);
    
    SelectObject(hdc, hOldBrush);
    SelectObject(hdc, hOldPen);
    DeleteObject(hBrush);
}

void MainWindow::DrawInfoItem(HDC hdc, const RECT& rect, const std::wstring& label, const std::wstring& value) {
    // Item background
    DrawRoundRect(hdc, rect, 8, m_colors.bgElevated, m_colors.borderSoft);

    // Label text
    SetTextColor(hdc, m_colors.textMuted);
    SetBkMode(hdc, TRANSPARENT);
    SelectObject(hdc, m_hFontLabel);
    TextOutW(hdc, rect.left + 14, rect.top + 8, label.c_str(), (int)label.length());

    // Value text
    SetTextColor(hdc, m_colors.textPrimary);
    SelectObject(hdc, m_hFontMono);

    // 如果卡片高度 > 60（会话ID），则支持多行显示
    int cardHeight = rect.bottom - rect.top;
    if (cardHeight > 60) {
        // 多行显示（使用 DrawText，自动换行）
        RECT valueRect = {
            rect.left + 14,
            rect.top + 26,
            rect.right - 14,
            rect.bottom - 6
        };
        DrawTextW(hdc, value.c_str(), (int)value.length(), &valueRect, 
                  DT_LEFT | DT_TOP | DT_WORDBREAK | DT_EDITCONTROL | DT_NOPREFIX);
    } else {
        // 单行显示，超长时省略
        RECT valueRect = {
            rect.left + 14,
            rect.top + 26,
            rect.right - 14,
            rect.bottom - 6
        };
        DrawTextW(hdc, value.c_str(), (int)value.length(), &valueRect, 
                  DT_LEFT | DT_TOP | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);
    }
}

void MainWindow::DrawTabBar(HDC hdc, const RECT& rect) {
    // Tab bar background
    HBRUSH hBgBrush = CreateBrush(m_colors.bgDeep);
    FillRect(hdc, &rect, hBgBrush);
    DeleteObject(hBgBrush);

    // Draw bottom border
    RECT borderRect = {rect.left, rect.bottom - 1, rect.right, rect.bottom};
    HBRUSH hBorderBrush = CreateBrush(m_colors.borderSoft);
    FillRect(hdc, &borderRect, hBorderBrush);
    DeleteObject(hBorderBrush);

    // Draw tabs
    int tabWidth = 120;
    int tabX = rect.left + 8;
    int tabHeight = rect.bottom - rect.top;

    const wchar_t* tabs[] = { L"运行日志", L"插件管理" };
    for (int i = 0; i < 2; i++) {
        RECT tabRect = {tabX, rect.top, tabX + tabWidth, rect.bottom};

        if (i == m_activeTab) {
            SetTextColor(hdc, RGB(0xFF, 0xFF, 0xFF));
            // Draw bottom border accent for active tab
            RECT accentRect = {tabRect.left + 8, tabRect.bottom - 2, tabRect.right - 8, tabRect.bottom};
            HBRUSH hAccentBrush = CreateBrush(m_colors.accent);
            FillRect(hdc, &accentRect, hAccentBrush);
            DeleteObject(hAccentBrush);
        } else {
            SetTextColor(hdc, m_colors.textSecondary);
        }

        SetBkMode(hdc, TRANSPARENT);
        SelectObject(hdc, m_hFont);

        // 使用固定位置居中
        int textX = tabRect.left + (tabWidth - 60) / 2;  // 假设文字约60像素宽
        int textY = tabRect.top + (tabHeight - 16) / 2;  // 假设文字高度16像素
        TextOutW(hdc, textX, textY, tabs[i], lstrlenW(tabs[i]));

        tabX += tabWidth;
    }
}

void MainWindow::DrawPluginPanel(HDC hdc, const RECT& rect) {
    // Plugin list area background
    DrawRoundRect(hdc, rect, 8, m_colors.bgDeep, m_colors.borderSoft);

    if (m_plugins.empty()) {
        // 显示空状态
        SetTextColor(hdc, m_colors.textMuted);
        SetBkMode(hdc, TRANSPARENT);
        SelectObject(hdc, m_hFont);
        const wchar_t* emptyText = L"暂无可管理的插件";
        int textLen = lstrlenW(emptyText);
        int textX = rect.left + (rect.right - rect.left - textLen * 13) / 2;
        int textY = rect.top + 48;
        TextOutW(hdc, textX, textY, emptyText, textLen);
    } else {
        // 绘制每个插件项
        for (size_t i = 0; i < m_pluginItemLayouts.size() && i < m_plugins.size(); i++) {
            DrawPluginItem(hdc, m_pluginItemLayouts[i], m_plugins[i], (int)i);
        }
    }
}

void MainWindow::DrawPluginItem(HDC hdc, const PluginItemLayout& layout, const PluginInstance& plugin, int index) {
    const RECT& rect = layout.itemRect;
    bool isHover = (index == m_hoverPluginIndex);

    // 背景：使用渐变近似（HTML: linear-gradient(135deg, rgba(28,34,56,0.85), rgba(20,25,42,0.85))）
    // 简化：用单一颜色填充，hover 时用稍亮颜色
    COLORREF bgColor = isHover ? RGB(0x24, 0x2a, 0x42) : RGB(0x1c, 0x22, 0x38);
    HBRUSH hBgBrush = CreateBrush(bgColor);
    FillRect(hdc, &rect, hBgBrush);
    DeleteObject(hBgBrush);

    // 边框
    COLORREF borderColor = isHover ? m_colors.borderStrong : m_colors.borderSoft;
    HBRUSH hBorderBrush = CreateBrush(borderColor);
    FrameRect(hdc, &rect, hBorderBrush);
    DeleteObject(hBorderBrush);

    // ===== 第一行：文件名 + 状态标签 + 优先级标签 =====
    int nameX = rect.left + 20;
    int nameY = rect.top + 16;
    
    // 第一行显示文件名（如 botpy_sdk.dll）
    std::wstring fileName = StringToWString(plugin.name);
    SetTextColor(hdc, m_colors.textPrimary);
    SetBkMode(hdc, TRANSPARENT);
    SelectObject(hdc, m_hPluginNameFont);  // 18px 粗体
    TextOutW(hdc, nameX, nameY, fileName.c_str(), (int)fileName.length());
    
    // 用 GetTextExtentPoint32 更精确计算文字宽度
    SIZE nameSize;
    GetTextExtentPoint32(hdc, fileName.c_str(), (int)fileName.length(), &nameSize);
    int nameWidth = nameSize.cx + 12;

    // 状态标签（启用=绿色，停用=红色）
    bool enabled = plugin.enabled;
    const wchar_t* statusText = enabled ? L"已启用" : L"已停用";
    COLORREF statusColor = enabled ? RGB(0x86, 0xef, 0xac) : RGB(0xfc, 0xa5, 0xa5);
    COLORREF statusBgColor = enabled ? m_colors.greenBg : m_colors.redBg;
    
    RECT statusRect = {
        nameX + nameWidth, nameY + 0,
        nameX + nameWidth + 72, nameY + 26
    };
    HBRUSH hStatusBg = CreateBrush(statusBgColor);
    FillRect(hdc, &statusRect, hStatusBg);
    DeleteObject(hStatusBg);
    HBRUSH hStatusBorder = CreateBrush(statusColor);
    FrameRect(hdc, &statusRect, hStatusBorder);
    DeleteObject(hStatusBorder);
    
    SetTextColor(hdc, statusColor);
    SelectObject(hdc, m_hPluginTagFont);   // 12px 标签字体
    SIZE statusSize;
    GetTextExtentPoint32(hdc, statusText, 3, &statusSize);
    int statusTextX = statusRect.left + (statusRect.right - statusRect.left - statusSize.cx) / 2;
    int statusTextY = statusRect.top + (statusRect.bottom - statusRect.top - statusSize.cy) / 2;
    TextOutW(hdc, statusTextX, statusTextY, statusText, 3);

    // 优先级标签（紫色）
    wchar_t priorityText[32];
    swprintf_s(priorityText, L"优先级: %d", plugin.priority);
    int priorityTextLen = lstrlenW(priorityText);
    
    SIZE prioritySize;
    SelectObject(hdc, m_hPluginTagFont);
    GetTextExtentPoint32(hdc, priorityText, priorityTextLen, &prioritySize);
    
    RECT priorityRect = {
        statusRect.right + 10, nameY + 0,
        statusRect.right + 10 + prioritySize.cx + 20, nameY + 26
    };
    COLORREF priorityColor = RGB(0xc7, 0xcb, 0xff);
    COLORREF priorityBgColor = RGB(0x1a, 0x1f, 0x3a);
    HBRUSH hPriorityBg = CreateBrush(priorityBgColor);
    FillRect(hdc, &priorityRect, hPriorityBg);
    DeleteObject(hPriorityBg);
    HBRUSH hPriorityBorder = CreateBrush(priorityColor);
    FrameRect(hdc, &priorityRect, hPriorityBorder);
    DeleteObject(hPriorityBorder);
    
    SetTextColor(hdc, priorityColor);
    SelectObject(hdc, m_hPluginTagFont);
    int priorityTextX = priorityRect.left + (priorityRect.right - priorityRect.left - prioritySize.cx) / 2;
    int priorityTextY = priorityRect.top + (priorityRect.bottom - priorityRect.top - prioritySize.cy) / 2;
    TextOutW(hdc, priorityTextX, priorityTextY, priorityText, priorityTextLen);

    // ===== 第二行：元数据（插件名、作者、简述）横向排列 =====
    int metaY = nameY + 42;
    int metaX = rect.left + 20;
    SelectObject(hdc, m_hPluginMetaFont);   // 14px 元数据字体
    SetTextColor(hdc, m_colors.textSecondary);

    // 插件名（display_name）
    if (!plugin.display_name.empty()) {
        std::wstring displayName = StringToWString(plugin.display_name);
        std::wstring metaText = L"插件名: " + displayName;
        TextOutW(hdc, metaX, metaY, metaText.c_str(), (int)metaText.length());
        SIZE metaSize;
        GetTextExtentPoint32(hdc, metaText.c_str(), (int)metaText.length(), &metaSize);
        metaX += metaSize.cx + 32;
    }

    // 作者
    if (!plugin.author.empty()) {
        std::wstring author = StringToWString(plugin.author);
        std::wstring metaText = L"作者: " + author;
        TextOutW(hdc, metaX, metaY, metaText.c_str(), (int)metaText.length());
        SIZE metaSize;
        GetTextExtentPoint32(hdc, metaText.c_str(), (int)metaText.length(), &metaSize);
        metaX += metaSize.cx + 32;
    }

    // 简述
    if (!plugin.description.empty()) {
        std::wstring desc = StringToWString(plugin.description);
        std::wstring metaText = L"简述: " + desc;
        // 限制简述长度，避免与按钮重叠
        int maxDescWidth = layout.toggleBtnRect.left - metaX - 20;
        SIZE metaSize;
        GetTextExtentPoint32(hdc, metaText.c_str(), (int)metaText.length(), &metaSize);
        if (metaSize.cx > maxDescWidth && maxDescWidth > 50) {
            // 估算省略后的字符数
            int avgCharWidth = metaSize.cx / (int)metaText.length();
            int maxChars = maxDescWidth / avgCharWidth;
            if (maxChars > 3 && (int)metaText.length() > maxChars) {
                metaText = metaText.substr(0, maxChars - 3) + L"...";
            }
        }
        TextOutW(hdc, metaX, metaY, metaText.c_str(), (int)metaText.length());
    }

    // ===== 右侧按钮（自绘） =====
    // 启用/停用按钮
    const wchar_t* toggleText = enabled ? L"停用" : L"启用";
    bool toggleHover = (isHover && m_hoverPluginButton == 1);
    
    COLORREF toggleBg = toggleHover ? m_colors.accent : RGB(0x2a, 0x2f, 0x4d);
    COLORREF toggleTextCol = toggleHover ? RGB(0xFF, 0xFF, 0xFF) : RGB(0xc7, 0xcb, 0xff);
    COLORREF toggleBorder = toggleHover ? m_colors.accent : RGB(0x3d, 0x45, 0x6d);
    
    const RECT& toggleRect = layout.toggleBtnRect;
    HBRUSH hToggleBg = CreateBrush(toggleBg);
    FillRect(hdc, &toggleRect, hToggleBg);
    DeleteObject(hToggleBg);
    HBRUSH hToggleBorder = CreateBrush(toggleBorder);
    FrameRect(hdc, &toggleRect, hToggleBorder);
    DeleteObject(hToggleBorder);
    
    SetTextColor(hdc, toggleTextCol);
    SelectObject(hdc, m_hPluginBtnFont);   // 14px 按钮字体
    int toggleTextLen = lstrlenW(toggleText);
    SIZE toggleSize;
    GetTextExtentPoint32(hdc, toggleText, toggleTextLen, &toggleSize);
    int toggleTextX = toggleRect.left + (toggleRect.right - toggleRect.left - toggleSize.cx) / 2;
    int toggleTextY = toggleRect.top + (toggleRect.bottom - toggleRect.top - toggleSize.cy) / 2;
    TextOutW(hdc, toggleTextX, toggleTextY, toggleText, toggleTextLen);

    // 卸载按钮
    const wchar_t* unloadText = L"卸载";
    bool unloadHover = (isHover && m_hoverPluginButton == 2);
    
    COLORREF unloadBg = unloadHover ? RGB(0xef, 0x44, 0x44) : m_colors.redBg;
    COLORREF unloadTextCol = unloadHover ? RGB(0xFF, 0xFF, 0xFF) : RGB(0xfc, 0xa5, 0xa5);
    COLORREF unloadBorder = unloadHover ? RGB(0xef, 0x44, 0x44) : RGB(0x5d, 0x2a, 0x2a);
    
    const RECT& unloadRect = layout.unloadBtnRect;
    HBRUSH hUnloadBg = CreateBrush(unloadBg);
    FillRect(hdc, &unloadRect, hUnloadBg);
    DeleteObject(hUnloadBg);
    HBRUSH hUnloadBorder = CreateBrush(unloadBorder);
    FrameRect(hdc, &unloadRect, hUnloadBorder);
    DeleteObject(hUnloadBorder);
    
    SetTextColor(hdc, unloadTextCol);
    SelectObject(hdc, m_hPluginBtnFont);   // 14px 按钮字体
    int unloadTextLen = lstrlenW(unloadText);
    SIZE unloadSize;
    GetTextExtentPoint32(hdc, unloadText, unloadTextLen, &unloadSize);
    int unloadTextX = unloadRect.left + (unloadRect.right - unloadRect.left - unloadSize.cx) / 2;
    int unloadTextY = unloadRect.top + (unloadRect.bottom - unloadRect.top - unloadSize.cy) / 2;
    TextOutW(hdc, unloadTextX, unloadTextY, unloadText, unloadTextLen);
}

void MainWindow::DrawPluginToolbar(HDC hdc, const RECT& rect) {
    // Toolbar background
    HBRUSH hBgBrush = CreateBrush(m_colors.bgDeep);
    FillRect(hdc, &rect, hBgBrush);
    DeleteObject(hBgBrush);

    // Draw bottom border
    RECT borderRect = {rect.left, rect.bottom - 1, rect.right, rect.bottom};
    HBRUSH hBorderBrush = CreateBrush(m_colors.borderSoft);
    FillRect(hdc, &borderRect, hBorderBrush);
    DeleteObject(hBorderBrush);
}

void MainWindow::OnPaint() {
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(m_hwnd, &ps);

    if (hdc) {
        // 绘制背景
        DrawBackground(hdc);

        // 绘制侧边栏
        DrawSidebar(hdc, m_sidebarRect);

        // 绘制内容区
        DrawContent(hdc, m_contentRect);

        // 绘制状态栏
        DrawStatusBar(hdc, m_statusBarRect);

        // 绘制信息项
        std::vector<std::pair<std::wstring, std::wstring>> items = {
            {L"AppID", m_appId},
            {L"昵称", m_nickname},
            {L"会话 ID", m_sessionId},
            {L"消息数", std::to_wstring(m_messageCount)},
            {L"心跳数", std::to_wstring(m_heartbeatCount)}
        };

        for (size_t i = 0; i < m_infoItemRects.size() && i < items.size(); i++) {
            DrawInfoItem(hdc, m_infoItemRects[i], items[i].first, items[i].second);
        }

        // 绘制选项卡
        DrawTabBar(hdc, m_tabBarRect);

        // 绘制插件管理面板（仅在插件管理选项卡时）
        if (m_activeTab == 1) {
            DrawPluginToolbar(hdc, m_pluginToolbarRect);
            DrawPluginPanel(hdc, m_pluginListAreaRect);
        }
    }

    EndPaint(m_hwnd, &ps);
}

void MainWindow::OnCommand(WPARAM wParam) {
    int ctrlId = LOWORD(wParam);
    int notifyCode = HIWORD(wParam);

    if (notifyCode == BN_CLICKED) {
        switch (ctrlId) {
            case IDC_PLUGIN_REFRESH:
                // 刷新插件：重新加载所有插件
                if (m_reloadPluginsFunc) {
                    // 使用 appid 作为参数（从 m_plugins 或配置获取）
                    // 这里简单地调用 reload，appid 参数实际未使用
                    m_reloadPluginsFunc("");
                    RefreshPluginList();
                } else {
                    // 回退：仅刷新列表显示
                    RefreshPluginList();
                }
                break;
        }
    }
}

void MainWindow::OnTabSelChange() {
    if (m_hTabControl) {
        m_activeTab = TabCtrl_GetCurSel(m_hTabControl);

        // 更新UI显示
        if (m_activeTab == 0) {
            // 日志选项卡
            if (m_hLogEdit) ShowWindow(m_hLogEdit, SW_SHOW);
            if (m_hPluginRefreshBtn) ShowWindow(m_hPluginRefreshBtn, SW_HIDE);
        } else {
            // 插件管理选项卡
            if (m_hLogEdit) ShowWindow(m_hLogEdit, SW_HIDE);
            if (m_hPluginRefreshBtn) ShowWindow(m_hPluginRefreshBtn, SW_SHOW);
            // 刷新插件列表
            RefreshPluginList();
        }
        InvalidateRect(m_hwnd, NULL, TRUE);
    }
}

void MainWindow::OnDestroy() {
    // 标记窗口已销毁，阻止后台线程继续发送消息
    m_windowDestroyed = true;

    // Destroy child windows first
    if (m_hLogEdit) DestroyWindow(m_hLogEdit);
    if (m_hTabControl) DestroyWindow(m_hTabControl);
    if (m_hPluginRefreshBtn) DestroyWindow(m_hPluginRefreshBtn);

    // Delete fonts
    if (m_hFont) DeleteObject(m_hFont);
    if (m_hFontBold) DeleteObject(m_hFontBold);
    if (m_hFontLabel) DeleteObject(m_hFontLabel);
    if (m_hFontMono) DeleteObject(m_hFontMono);
    if (m_hFontTitle) DeleteObject(m_hFontTitle);
    if (m_hFontSmall) DeleteObject(m_hFontSmall);
    if (m_hPluginNameFont) DeleteObject(m_hPluginNameFont);
    if (m_hPluginMetaFont) DeleteObject(m_hPluginMetaFont);
    if (m_hPluginTagFont) DeleteObject(m_hPluginTagFont);
    if (m_hPluginBtnFont) DeleteObject(m_hPluginBtnFont);
}

void MainWindow::OnLButtonDown(int x, int y) {
    // 检查是否在插件列表区域点击
    if (m_activeTab == 1 && m_hwnd) {
        // x, y 已经是客户区坐标（来自 WM_LBUTTONDOWN 的 lParam）
        POINT pt = {x, y};

        // 检查是否点击在插件项的按钮上
        for (size_t i = 0; i < m_pluginItemLayouts.size() && i < m_plugins.size(); i++) {
            const PluginItemLayout& layout = m_pluginItemLayouts[i];
            
            // 检查启用/停用按钮
            if (PtInRect(&layout.toggleBtnRect, pt)) {
                const std::string& pluginName = m_plugins[i].name;
                bool currentEnabled = m_plugins[i].enabled;
                if (m_togglePluginFunc) {
                    m_togglePluginFunc(pluginName, !currentEnabled);
                    RefreshPluginList();
                }
                return;
            }
            
            // 检查卸载按钮
            if (PtInRect(&layout.unloadBtnRect, pt)) {
                const std::string& pluginName = m_plugins[i].name;
                // 弹出确认对话框
                std::wstring wname = StringToWString(pluginName);
                std::wstring msg = L"确定要卸载插件 " + wname + L" 吗？";
                if (MessageBoxW(m_hwnd, msg.c_str(), L"确认卸载", MB_YESNO | MB_ICONQUESTION) == IDYES) {
                    if (m_unloadPluginFunc) {
                        m_unloadPluginFunc(pluginName);
                        RefreshPluginList();
                    }
                }
                return;
            }
        }
    }
}

void MainWindow::SetConnected(bool connected) {
    m_isConnected = connected;
    InvalidateRect(m_hwnd, NULL, TRUE);
}

void MainWindow::SetAppId(const std::wstring& appId) {
    m_appId = appId;
    InvalidateRect(m_hwnd, NULL, TRUE);
}

void MainWindow::SetNickname(const std::wstring& nickname) {
    m_nickname = nickname;
    InvalidateRect(m_hwnd, NULL, TRUE);
}

void MainWindow::SetSessionId(const std::wstring& sessionId) {
    m_sessionId = sessionId;
    InvalidateRect(m_hwnd, NULL, TRUE);
}

void MainWindow::SetMessageCount(int count) {
    m_messageCount = count;
    InvalidateRect(m_hwnd, NULL, TRUE);
}

void MainWindow::SetHeartbeatCount(int count) {
    m_heartbeatCount = count;
    InvalidateRect(m_hwnd, NULL, TRUE);
}
void MainWindow::LimitLogLines(int maxLines) {
    // 获取所有文本
    int textLen = GetWindowTextLengthW(m_hLogEdit);
    if (textLen == 0) return;

    std::wstring text(textLen, L'\0');
    GetWindowTextW(m_hLogEdit, &text[0], textLen + 1);

    // 统计行数
    int lineCount = 0;
    for (size_t i = 0; i < text.length(); ++i) {
        if (text[i] == L'\n') lineCount++;
    }

    // 如果超过最大行数，删除最旧的行
    if (lineCount > maxLines) {
        // 找到需要删除的行数
        int linesToDelete = lineCount - maxLines;
        size_t deletePos = 0;

        for (int i = 0; i < linesToDelete; ++i) {
            size_t pos = text.find(L'\n', deletePos);
            if (pos != std::wstring::npos) {
                deletePos = pos + 1;  // 包含换行符
            }
        }

        // 删除旧行
        text.erase(0, deletePos);

        // 更新控件
        SetWindowTextW(m_hLogEdit, text.c_str());
    }
}
void MainWindow::AppendLog(const std::wstring& message, const std::wstring& type) {
    if (!m_hLogEdit) return;

    time_t now = time(nullptr);
    struct tm timeInfo = {};
    localtime_s(&timeInfo, &now);
    wchar_t timeStr[32];
    wcsftime(timeStr, sizeof(timeStr)/sizeof(wchar_t), L"[%H:%M:%S]", &timeInfo);

    std::wstring logEntry = std::wstring(timeStr) + L" " + message + L"\r\n";

    // 获取当前文本长度
    int textLen = GetWindowTextLengthW(m_hLogEdit);
    
    // 选择末尾
    SendMessage(m_hLogEdit, EM_SETSEL, textLen, textLen);
    
    // 追加文本
    SendMessage(m_hLogEdit, EM_REPLACESEL, FALSE, (LPARAM)logEntry.c_str());
    LimitLogLines(100);
    // 滚动到底部
    SendMessage(m_hLogEdit, EM_SCROLLCARET, 0, 0);
}

// ============ 机器人回调接口实现 ============

// 智能字符串转换：先尝试UTF-8，如果失败则使用ANSI(GB2312)
static std::wstring SmartStringToWString(const std::string& str) {
    if (str.empty()) return L"";
    
    // 先尝试UTF-8
    int wlen = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, NULL, 0);
    if (wlen > 0) {
        std::wstring wstr(wlen - 1, L'\0');
        int result = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &wstr[0], wlen);
        if (result > 0) {
            // 检查是否包含无效的Unicode字符（如果有替换字符，说明可能不是UTF-8）
            bool has_invalid = false;
            for (wchar_t c : wstr) {
                if (c == 0xFFFD) { // Unicode替换字符
                    has_invalid = true;
                    break;
                }
            }
            if (!has_invalid) {
                return wstr;
            }
        }
    }
    
    // UTF-8失败或包含无效字符，使用ANSI (GB2312)
    wlen = MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, NULL, 0);
    if (wlen <= 0) return L"";
    std::wstring wstr(wlen - 1, L'\0');
    MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, &wstr[0], wlen);
    return wstr;
}

void MainWindow::PostLog(const std::string& level, const std::string& msg) {
    if (m_windowDestroyed || !m_hwnd) return;
    UILogMsg* uiMsg = new UILogMsg();
    uiMsg->level = level;
    uiMsg->msg = msg;
    PostMessage(m_hwnd, WM_UI_LOG, 0, (LPARAM)uiMsg);
}

void MainWindow::PostLogUtf8(const std::string& level, const std::string& msg) {
    PostLog(level, msg);
}

void MainWindow::PostStatus(const std::string& status, const std::string& text) {
    if (m_windowDestroyed || !m_hwnd) return;
    UIStatusMsg* uiMsg = new UIStatusMsg();
    uiMsg->status = status;
    uiMsg->text = text;
    PostMessage(m_hwnd, WM_UI_STATUS, 0, (LPARAM)uiMsg);
}

void MainWindow::PostInfo(const std::string& id, const std::string& value) {
    if (m_windowDestroyed || !m_hwnd) return;
    UIInfoMsg* uiMsg = new UIInfoMsg();
    uiMsg->id = id;
    uiMsg->value = value;
    PostMessage(m_hwnd, WM_UI_INFO, 0, (LPARAM)uiMsg);
}

void MainWindow::PostMessageEvent(bool is_group, const std::string& content) {
    if (m_windowDestroyed || !m_hwnd) return;
    UIMessageEventMsg* uiMsg = new UIMessageEventMsg();
    uiMsg->is_group = is_group;
    uiMsg->content = content;
    PostMessage(m_hwnd, WM_UI_MESSAGE_EVENT, 0, (LPARAM)uiMsg);
}

void MainWindow::PostPlugins(const std::string& json_str) {
    if (m_windowDestroyed || !m_hwnd) return;
    UIPluginsMsg* uiMsg = new UIPluginsMsg();
    uiMsg->json_str = json_str;
    PostMessage(m_hwnd, WM_UI_PLUGINS_UPDATE, 0, (LPARAM)uiMsg);
}

// ============ Thread-safe UI message handlers (called from UI thread) ============

void MainWindow::HandleUILogMsg(UILogMsg* msg) {
    if (!msg) return;
    std::wstring wlevel = StringToWString(msg->level);
    std::wstring wmsg = SmartStringToWString(msg->msg);
    AppendLog(wmsg, wlevel);
    delete msg;
}

void MainWindow::HandleUIStatusMsg(UIStatusMsg* msg) {
    if (!msg) return;
    if (msg->status == "connected" || msg->status == "online") {
        SetConnected(true);
        SetSessionId(SmartStringToWString(msg->text));
    } else if (msg->status == "disconnected") {
        SetConnected(false);
    } else {
        // 其他状态也记录日志
        std::wstring wlevel = L"status";
        std::wstring wmsg = SmartStringToWString(msg->text);
        AppendLog(wmsg, wlevel);
    }
    delete msg;
}

void MainWindow::HandleUIInfoMsg(UIInfoMsg* msg) {
    if (!msg) return;
    if (msg->id == "appId") {
        SetAppId(SmartStringToWString(msg->value));
    } else if (msg->id == "nickname" || msg->id == "nick_name") {
        SetNickname(SmartStringToWString(msg->value));
    } else if (msg->id == "heartbeatCount") {
        try {
            SetHeartbeatCount(std::stoi(msg->value));
        } catch (...) {}
    } else if (msg->id == "sessionId") {
        SetSessionId(SmartStringToWString(msg->value));
    } else if (msg->id == "msgCount") {
        try {
            SetMessageCount(std::stoi(msg->value));
        } catch (...) {}
    }
    delete msg;
}

void MainWindow::HandleUIMessageEventMsg(UIMessageEventMsg* msg) {
    if (!msg) return;
    std::wstring prefix = msg->is_group ? L"[群] " : L"[私聊] ";
    std::wstring wcontent = SmartStringToWString(msg->content);
    AppendLog(prefix + wcontent, L"message");
    SetMessageCount(m_messageCount + 1);
    delete msg;
}

void MainWindow::HandleUIPluginsMsg(UIPluginsMsg* msg) {
    if (!msg) return;
    RefreshPluginList();
    PostLog("info", "插件列表已更新");
    delete msg;
}

void MainWindow::SetPluginGetHandler(GetPluginsFunc func) {
    m_getPluginsFunc = func;
}

void MainWindow::SetPluginToggleHandler(TogglePluginFunc func) {
    m_togglePluginFunc = func;
}

void MainWindow::SetPluginUnloadHandler(UnloadPluginFunc func) {
    m_unloadPluginFunc = func;
}

void MainWindow::SetPluginReloadHandler(ReloadPluginsFunc func) {
    m_reloadPluginsFunc = func;
}

void MainWindow::RefreshPluginList() {
    if (m_getPluginsFunc) {
        m_plugins = m_getPluginsFunc();
        CalculatePluginLayout();
        InvalidateRect(m_hwnd, NULL, TRUE);
    }
}

void MainWindow::SetCommandHandler(std::function<void(const std::string&)> handler) {
    m_commandHandler = handler;
}

void MainWindow::SetReadyHandler(std::function<void()> handler) {
    m_readyHandler = handler;
}

void MainWindow::RunMessageLoop() {
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

std::wstring MainWindow::StringToWString(const std::string& str) {
    if (str.empty()) return L"";
    // 使用 CP_ACP (当前ANSI代码页，中文系统上为GB2312/GBK)
    int wlen = MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, NULL, 0);
    if (wlen <= 0) return L"";
    std::wstring wstr(wlen - 1, L'\0');
    MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, &wstr[0], wlen);
    return wstr;
}

// UTF-8 转宽字符（用于机器人传来的 UTF-8 消息）
std::wstring MainWindow::Utf8ToWString(const std::string& str) {
    if (str.empty()) return L"";
    int wlen = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, NULL, 0);
    if (wlen <= 0) {
        // 如果 UTF-8 转换失败，尝试使用 ANSI 代码页
        return StringToWString(str);
    }
    std::wstring wstr(wlen - 1, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &wstr[0], wlen);
    return wstr;
}
