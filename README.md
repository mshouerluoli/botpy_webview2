# 🐱 Miao Bot (botpy_WebView2)

基于 C++ 开发的 QQ 机器人客户端，通过 WebSocket 连接 QQ 官方机器人网关，提供 GUI 界面实时展示运行状态和日志，并内置**插件系统**支持动态加载第三方功能模块。

本项目提供**两个版本**，共享同一套核心逻辑与插件 API，仅界面渲染层不同，可根据部署环境选择：

## 📦 版本说明

| 版本 | 路径 | 界面技术 | WebView2 依赖 | 适用场景 |
|------|------|----------|---------------|----------|
| **桌面版** | `botpy_WebView2/` | WebView2 + HTML/CSS/JS | ✅ 需要 | 个人桌面、开发调试，追求现代毛玻璃 UI |
| **服务器版** | `ServerVersion/Botpy_WindowEx/` | 原生 Win32 GDI 控件 | ❌ 无需 | 服务器 / 无 WebView2 Runtime 的精简环境 |

### 桌面版（botpy_WebView2）

- 使用 WebView2 渲染 HTML/CSS/JS 界面，毛玻璃质感 + 渐变光晕的现代暗色主题
- HTML 以 RCDATA 资源嵌入 EXE，启动时自动释放到 `ui/index.html`
- 需要目标机器预装 WebView2 Runtime（Windows 10/11 通常已预装）
- C++17

### 服务器版（Botpy_WindowEx）

- 使用**原生 Win32 控件 + GDI 自绘**实现界面（RichEdit 日志框、TabControl 标签页、自绘侧边栏与插件卡片）
- **不依赖 WebView2 Runtime**，单文件 EXE 即可运行，适合服务器或精简部署环境
- 主题颜色在 C++ 代码中硬编码（`ThemeColors` 结构体），无需外部 HTML 资源
- 额外包含 `http/winnet_helper.hpp`（WinINet 辅助库）
- C++20
- 与桌面版共享 `Miao.cpp`、`message_queue.h`、`plugin_api.h`、`http/`、`websocket/` 等核心代码，**插件 DLL 完全通用**

> ⚠️ **开发主线说明**：若无特殊说明，**`ServerVersion/`（服务器版）才是最新主线版本**，包含最新的插件 API（`is_groupat` 字段、HTTP GET/POST 回调等）。桌面版 `botpy_WebView2/` 的代码与插件 API 暂未同步更新，可能落后于服务器版。本 README 中的**插件 API 说明、结构体定义、接口签名，如未单独标注，均以 `ServerVersion/Botpy_WindowEx/Botpy_WindowEx/plugin_api.h` 为准**。
>
> 两个版本的 `config.yaml` 格式、`plugins/` 与 `plugin_data/` 目录约定一致；但**旧桌面版缺少新增字段与回调**，若插件使用了新 API，需要在 `plugin_init` 中做空指针判断（如 `if (params->http_get_func) {...}`），以便兼容旧版宿主。

## ✨ 功能特性

- **GUI 界面**：桌面版使用 WebView2 渲染 HTML/CSS/JS（毛玻璃质感 + 渐变光晕的现代暗色主题）；服务器版使用原生 Win32 GDI 自绘界面，无 WebView2 依赖
- **WebSocket 通信**：基于 WinHttpWebSocket API 连接 QQ 机器人网关
- **自动重连**：连接断开后自动重新认证并重连，无需人工干预
- **心跳保活**：定时发送心跳包，检测连接状态，超时自动重连
- **消息处理**：支持私聊消息（C2C_MESSAGE_CREATE）和群@消息（GROUP_AT_MESSAGE_CREATE）
- **文件发送**：支持向私聊和群聊发送图片、视频、音频、文件（通过 URL 上传）
- **实时日志**：所有运行日志、状态变化、收发消息实时显示在 UI 中，自动换行、横向滚动条隐藏
- **资源嵌入**（桌面版）：HTML 界面以 RCDATA 资源形式嵌入 EXE，启动时自动释放，无需额外文件
- **⚡ 多线程消息处理**：基于消息队列 + 工作线程池的生产者-消费者模型，4 个工作线程并发处理消息，高并发下不阻塞 WebSocket 接收
- **🔌 插件系统**：支持动态加载 DLL 插件，提供独立的插件数据目录、优先级调度、消息拦截机制，可在 UI 中启用/停用/卸载/重载

## 🛠️ 技术栈

| 组件 | 桌面版 | 服务器版 |
|------|--------|----------|
| 语言 | C++ (C++17) | C++ (C++20) |
| UI | WebView2 + HTML/CSS/JS | 原生 Win32 控件 + GDI 自绘 |
| WebSocket | WinHttpWebSocket API | WinHttpWebSocket API |
| HTTP | WinHTTP | WinHTTP（含 WinINet 辅助） |
| JSON | nlohmann/json | nlohmann/json |
| 插件接口 | C ABI（DLL 导出函数） | C ABI（DLL 导出函数） |
| 构建 | Visual Studio 2022 | Visual Studio 2022 |
| 平台 | Windows (x64) | Windows (x64) |

## 📁 项目结构

```
botpy_WebView2/
├── README.md
├── .gitignore
├── 推送.bat                         # 部署/推送辅助脚本
├── Miao.py                          # Python 参考脚本（对应协议与 API 流程）
├── botpy_WebView2.slnx              # 🖥️ 桌面版解决方案（旧，暂未同步 ServerVersion 新 API）
├── botpy_WebView2/                  # 桌面版主程序（WebView2 + HTML/CSS/JS）
│   ├── botpy_WebView2.vcxproj       # VS 项目文件
│   ├── botpy_WebView2.vcxproj.filters
│   ├── botpy_WebView2.rc            # 资源文件（嵌入 ui/index.html 为 RCDATA）
│   ├── resource.h                   # 资源 ID 头文件
│   ├── plugin_api.h                 # ⚠️ 插件 API（桌面版副本，较旧；以 ServerVersion/ 下的为主线）
│   ├── message_queue.h              # ⚡ 线程安全消息队列（工作线程池用）
│   ├── MainWindow.h / MainWindow.cpp         # 主窗口 + WebView2 初始化/管理
│   ├── Miao.h / Miao.cpp                     # 机器人核心逻辑 + 插件管理器
│   ├── webview_handlers.h / webview_handlers.cpp  # WebView2 事件（消息、导航等）
│   ├── json.hpp                     # nlohmann/json（单头文件库）
│   ├── ui/
│   │   └── index.html               # 前端界面（桌面版独有，编译时嵌入 EXE）
│   ├── http/
│   │   ├── websocket_client.hpp / .cpp  # WebSocket 客户端（WinHttpWebSocket）
│   │   ├── winhttp_helper.hpp           # WinHTTP 请求构造/发送辅助
│   │   ├── winnet_helper.hpp            # WinINet 辅助库（LowBoyTeam）
│   │   ├── restclient.hpp               # REST API 客户端高层封装
│   │   └── textconv_helper.hpp          # 宽字节 / UTF-8 编码转换
│   └── websocket/
│       ├── websocket.h  / .cpp          # WebSocket 帧打包/解包底层封装
│
├── ServerVersion/
│   └── Botpy_WindowEx/              # 🌟 服务器版 / 最新主线（原生 Win32 GDI，无 WebView2 依赖）
│       ├── Botpy_WindowEx.slnx      # 服务器版解决方案
│       └── Botpy_WindowEx/
│           ├── Botpy_WindowEx.vcxproj
│           ├── Botpy_WindowEx.vcxproj.filters
│           ├── main.cpp             # 入口（WinMain）：装配回调、启动 bot 线程、消息循环
│           ├── MainWindow.h / MainWindow.cpp   # 原生 Win32 窗口 + GDI 自绘界面（ThemeColors 定义主题）
│           ├── Miao.h / Miao.cpp               # 核心逻辑（含 plugin_http_get_wrapper / plugin_http_post_wrapper）
│           ├── config.yaml          # 示例配置文件（开发期拷贝参考）
│           ├── plugin_api.h         # 🌟 插件 API 主线副本（含 is_groupat、http_get/post_func）
│           ├── message_queue.h      # 线程安全消息队列（同源）
│           ├── resource.h
│           ├── json.hpp
│           ├── http/
│           │   ├── websocket_client.hpp / .cpp
│           │   ├── winhttp_helper.hpp
│           │   ├── winnet_helper.hpp           # WinINet 辅助
│           │   ├── python_http_helper.hpp      # 🌟 服务器版独有：PythonHttpClient（HTTP GET/POST 回调底层实现）
│           │   ├── restclient.hpp
│           │   └── textconv_helper.hpp
│           └── websocket/
│               ├── websocket.h / .cpp
│
└── sdk/
    └── botpy_sdk/                   # 🔌 插件 SDK（示例插件工程）
        ├── botpy_sdk.slnx
        └── botpy_sdk/
            ├── botpy_sdk.vcxproj
            ├── botpy_sdk.vcxproj.filters
            ├── framework.h          # VS 默认预编译框架头
            ├── dllmain.cpp          # 示例插件：导出 init/handle_message/shutdown、发送/HTTP 示例
            ├── plugin_api.h         # 插件 API 头（编译插件使用，需与宿主保持一致）
            └── json.hpp             # nlohmann/json（示例中用于解析 HTTP 回调返回值）
```

> **关于版本与副本**：`plugin_api.h` / `Miao.*` 在桌面版、服务器版、SDK 下各有一份。若出现字段/签名不一致，**一律以 `ServerVersion/Botpy_WindowEx/Botpy_WindowEx/` 下的副本为最新主线**（见 [版本说明](#-版本说明) 中的开发主线提示）。

## 📋 配置说明

在 EXE 同目录下创建 `config.yaml` 文件：

```yaml
appid: "你的机器人AppID"
secret: "你的机器人Secret"
worker_count: 4    # 可选，工作线程数，默认 4，范围 1~64
```

> AppID 和 Secret 可在 [QQ 开放平台](https://q.qq.com/) 申请。

## 🚀 编译与运行

### 编译要求

- Visual Studio 2022（含 C++ 桌面开发工作负载）
- Windows SDK
- 桌面版另需：WebView2 Runtime（Windows 10/11 通常已预装）+ WebView2 SDK（`WebView2.h`）
- 服务器版：无 WebView2 依赖

### 编译步骤

**桌面版（botpy_WebView2）**

1. 用 Visual Studio 打开 `botpy_WebView2.slnx`
2. 选择 `Release` | `x64` 配置
3. 生成解决方案（`Ctrl+Shift+B`）

**服务器版（Botpy_WindowEx）**

1. 用 Visual Studio 打开 `ServerVersion/Botpy_WindowEx/Botpy_WindowEx.slnx`
2. 选择 `Release` | `x64` 配置
3. 生成解决方案（`Ctrl+Shift+B`）

### 运行步骤

1. 将生成的 EXE（桌面版 `botpy_WebView2.exe` 或服务器版 `Botpy_WindowEx.exe`）复制到任意目录
2. 在同目录下创建 `config.yaml` 并填入 AppID 和 Secret
3. 运行对应的 EXE

程序启动后会自动：

- （桌面版）从资源中释放 `ui/index.html` 到运行目录的 `ui/` 子目录，并初始化 WebView2 加载界面
- （服务器版）创建原生 Win32 窗口并自绘界面
- 读取 `config.yaml` 配置
- 创建 `plugins/` 与 `plugin_data/` 目录（不存在时）
- 加载 `plugins/` 下所有插件 DLL
- 连接 QQ 机器人网关并开始监听消息

## 🖥️ 界面说明

界面采用暗色风格，分为左侧信息栏和右侧标签页区。两个版本界面布局一致，桌面版由 WebView2 渲染 HTML/CSS（毛玻璃质感 + 渐变光晕），服务器版由 Win32 GDI 自绘（实色暗色主题）。

**机器人信息（侧边栏）**

- 顶部状态条：彩色脉冲指示灯（在线/连接中/离线）+ 状态文字
- AppID：机器人应用 ID
- 昵称：机器人账号昵称（连接成功后显示）
- 会话 ID：WebSocket 会话 ID
- 消息数：累计收到的消息数量
- 心跳数：累计发送的心跳次数

**运行日志（标签页）**

- 实时显示连接状态、认证过程、心跳、消息收发等日志
- 不同级别日志以不同颜色区分（info / success / warning / error / message）
- 收到消息时显示 `[群]` 或 `[私聊]` 前缀及消息内容
- 超过 100 条自动清理最旧记录，长日志自动换行（隐藏横向滚动条）

**插件管理（标签页）**

- 以卡片形式展示所有已加载插件，含名称、状态徽章、优先级、作者、简述
- 每个插件可单独 **启用 / 停用 / 卸载**
- 底部提供「重新加载所有插件」按钮

## 🔌 插件系统

Miao Bot 通过 C ABI 的 DLL 导出函数加载第三方插件，插件可接收消息、调用主程序 API 发送消息、写日志，并拥有独立的数据目录。

### 插件目录约定

```
<exe所在目录>/
├── botpy_WebView2.exe        # 或 Botpy_WindowEx.exe（服务器版）
├── config.yaml
├── plugins/                 # 插件 DLL 放这里
│   ├── my_plugin.dll
│   └── another.dll
└── plugin_data/             # 自动生成，每个插件一个数据子目录
    ├── my_plugin.dll/       # 名称与 DLL 文件名一致
    └── another.dll/
```

> 启动时主程序会自动创建 `plugins/` 和 `plugin_data/`，并为每个插件在其 `plugin_data/<dll文件名>/` 下创建独立数据目录，通过 `PluginInitParams.data_path` 传给插件。

### 插件需要导出的函数

在 [ServerVersion/Botpy_WindowEx/Botpy_WindowEx/plugin_api.h](ServerVersion/Botpy_WindowEx/Botpy_WindowEx/plugin_api.h) 中定义（这是最新主线版本）。**必选**导出：

| 函数签名 | 说明 |
|---------|------|
| `int plugin_init(const PluginInitParams* params)` | 初始化，返回 0 表示成功（非 0 则加载失败被丢弃） |
| `int plugin_handle_message(const PluginMessage* msg)` | 处理一条消息，返回 1 拦截后续低优先级插件，返回 0 继续 |

**可选**导出：

| 函数签名 | 说明 |
|---------|------|
| `void plugin_shutdown()` | 卸载时回调，用于释放资源 |
| `const char* plugin_get_name()` | 插件显示名 |
| `int plugin_get_priority()` | 优先级（数值越小优先级越高，默认 0） |
| `const char* plugin_get_author()` | 作者名 |
| `const char* plugin_get_description()` | 插件简介 |

### 插件 API 数据结构

```c
// 主程序传给插件的消息结构
typedef struct {
    const char* id;           // 消息 ID
    const char* content;      // 消息文本内容
    const char* sender_id;    // 发送者 ID
    const char* channel_id;   // 频道 ID
    int is_group;             // 1=群消息, 0=私聊
    int is_groupat;           // 1=群@消息（@机器人）, 0=普通群消息 / 私聊（ServerVersion 新增）
    const char* openid;       // 私聊对方 openid
    const char* group_openid; // 群 openid
} PluginMessage;

// 初始化时主程序传入的参数
typedef struct {
    PluginLogFunc log_func;          // 日志输出函数
    PluginSendMessageFunc send_msg_func;   // 发送消息函数
    PluginPostFileFunc post_file_func;     // 发送文件函数
    PluginHttpGetFunc http_get_func;       // HTTP GET 请求函数（ServerVersion 新增，旧版为 NULL，请做空指针判断）
    PluginHttpPostFunc http_post_func;     // HTTP POST 请求函数（ServerVersion 新增，旧版为 NULL，请做空指针判断）
    const char* appid;               // 当前机器人 AppID
    const char* data_path;           // 插件专属数据目录路径
} PluginInitParams;
```

### 优先级与消息拦截

- 插件按 `plugin_get_priority()` 返回值**升序**处理（数值越小越先收到消息）
- 高优先级插件若在 `plugin_handle_message` 中 `return 1`，则后续低优先级插件**不会**收到该消息
- 返回 0 表示放行，消息继续传递给下一个插件
- 已停用（`enabled = false`）的插件不参与消息分发

### 最小示例

参见 [sdk/botpy_sdk/botpy_sdk/dllmain.cpp](sdk/botpy_sdk/botpy_sdk/dllmain.cpp)（完整版，含 HTTP GET/POST 封装与 `is_groupat` 判断）。核心骨架：

```c
#include "plugin_api.h"
#include <string>

static PluginLogFunc        g_log       = nullptr;
static PluginSendMessageFunc g_send_msg = nullptr;
static PluginPostFileFunc   g_post_file = nullptr;
// ServerVersion 新增 HTTP 回调（旧桌面版中保持 NULL，调用前必须判空）
static PluginHttpGetFunc    g_http_get  = nullptr;
static PluginHttpPostFunc   g_http_post = nullptr;
static std::string data_dir;

extern "C" __declspec(dllexport) int plugin_init(const PluginInitParams* params) {
    g_log       = params->log_func;
    g_send_msg  = params->send_msg_func;
    g_post_file = params->post_file_func;
    g_http_get  = params->http_get_func;    // 可能为 NULL（旧桌面版）
    g_http_post = params->http_post_func;   // 可能为 NULL（旧桌面版）
    if (params->data_path) data_dir = params->data_path;
    if (g_log) {
        g_log("info", "MyPlugin loaded");
        if (g_http_get)  g_log("info",  "HTTP GET API available");
        else             g_log("warn",  "HTTP GET API NOT available (old desktop host?)");
        if (g_http_post) g_log("info", "HTTP POST API available");
        else             g_log("warn", "HTTP POST API NOT available (old desktop host?)");
    }
    return 0;                            // 0 = 加载成功
}

extern "C" __declspec(dllexport) int plugin_handle_message(const PluginMessage* msg) {
    if (!msg) return 0;

    // is_groupat：仅 ServerVersion 填充，1=群里有人 @ 了机器人
    if (msg->is_group && msg->group_openid) {
        if (msg->is_groupat) {
            // 群@被动回复：msg_id 必填（使用 msg->id）
            g_send_msg(msg->group_openid, "群@回复示例", 1, msg->id, 0, "");
        }

        // HTTP GET 示例（调用前必须判空！）
        if (g_http_get) {
            const char* resp = g_http_get(
                "https://httpbin.org/get",
                "{\"User-Agent\":\"MyPlugin/1.0\"}");
            // resp 为 JSON 字符串：{"success","status_code","body","error"}
        }

        // HTTP POST 示例（调用前必须判空！）
        if (g_http_post) {
            const char* resp = g_http_post(
                "https://httpbin.org/post",
                "{\"hello\":\"world\"}",
                "{\"Content-Type\":\"application/json\"}");
        }
    } else if (!msg->is_group && msg->openid) {
        // 私聊：msg_id 可传空字符串
        g_send_msg(msg->openid, "私聊回复示例", 0, "", 0, "");
    }
    return 0;                               // 0 = 放行，1 = 拦截不再向下传递
}

extern "C" __declspec(dllexport) void plugin_shutdown() {
    if (g_log) g_log("info", "MyPlugin unloaded");
}

extern "C" __declspec(dllexport) const char* plugin_get_name()        { return "MyPlugin"; }
extern "C" __declspec(dllexport) int          plugin_get_priority()   { return 100; }
extern "C" __declspec(dllexport) const char* plugin_get_author()      { return "Miaopasi"; }
extern "C" __declspec(dllexport) const char* plugin_get_description() { return "示例插件（兼容新老宿主）"; }
```

> **兼容性提示**：`is_groupat`、`http_get_func`、`http_post_func` 是 ServerVersion 的新增字段/回调。
> 旧桌面版中 `PluginMessage.is_groupat` 字段所在内存位置不存在（结构体更小），因此**读取到的值是未定义的**；`params->http_get_func/http_post_func` 在旧宿主中为 `NULL`。
> 如插件需要兼容两版宿主，请：
> 1. HTTP 回调使用前先 `if (g_http_get) {...}` 判空再调用；
> 2. `is_groupat` 仅在 `is_group==1` 时使用或自行做空值兼容判断。

编译为 DLL 后放入 `plugins/` 目录，启动主程序即可自动加载；也可在「插件管理」标签页中热重载。

## 🔧 核心机制

### 连接流程

1. **认证**：POST `/app/getAppAccessToken` 获取 access_token
2. **获取网关**：GET `/gateway` 获取 WebSocket 地址
3. **连接 WebSocket**：建立 TLS 安全连接
4. **Identify**：发送 op=2 鉴权包，intents 使用 `33558528`
5. **心跳保活**：按服务端指定间隔发送 op=1 心跳包

### 自动重连

- 检测到连接断开后，等待 5 秒自动重连
- 重连流程：重新认证 → 获取网关 → 连接 WebSocket → 发送 Identify
- 心跳超时检测：超过 2 倍心跳间隔未收到消息则判定为断开
- 心跳线程在重新创建前会先 detach 旧线程，避免 joinable 线程被覆盖导致 `std::terminate`
- 重连时消息队列会重置状态，工作线程池会完整停止后重新启动，确保重连后消息处理正常

### 消息分发

收到 QQ 消息后采用**生产者-消费者模型**处理：

1. **WebSocket 接收线程（生产者）**：解析事件，更新 UI（消息数 +1，日志打印），将消息封装为任务推入消息队列后立即返回，继续接收下一条消息
2. **工作线程池（消费者）**：4 个工作线程从消息队列并发取任务，每个任务内部按优先级升序将消息分发给所有已启用的插件
3. 任一插件返回 1 即停止后续分发（同一条消息内的插件链保持串行，拦截机制完全有效）
4. 插件可通过 `send_msg_func` 主动发送回复消息

> 单条消息内部的插件处理链仍然串行执行，保证优先级排序和消息拦截逻辑正确；并发体现在**多条消息之间**同时处理。

### 消息回复 API

插件调用 `send_msg_func(target_id, content, is_group, msg_id, msg_type, media)`：

- 私聊：POST `/v2/users/{openid}/messages`
- 群聊：POST `/v2/groups/{group_openid}/messages`
- `msg_id`：群@消息被动回复必填，私聊可传空字符串
- `msg_type`：消息类型，0=文本 2=Markdown 3=Ark消息 4=Embed消息 7=Media
- `media`：媒体文件信息，文本消息可传空字符串

### 文件发送 API

插件调用 `post_file_func(target_id, url, file_type, is_group, srv_send_msg)`：

- 私聊文件：POST `/v2/users/{openid}/files`
- 群聊文件：POST `/v2/groups/{group_openid}/files`
- `target_id`：对方 openid（私聊）或群 openid（群聊）
- `url`：文件 URL 地址
- `file_type`：文件类型（1=图片, 2=视频, 3=音频, 4=文件）
- `is_group`：1=群聊, 0=私聊
- `srv_send_msg`：是否由服务端直接发送消息，1=是, 0=否
- 返回值：QQ 平台返回的 JSON 字符串（失败返回空字符串）

### HTTP 请求 API（ServerVersion 新增）

插件调用 `http_get_func(url, headers_json)` 或 `http_post_func(url, data, headers_json)` 发起 HTTP 请求，**无需自己引入 libcurl / WinHTTP**。使用前需在 `plugin_init` 中做空指针判断（旧桌面版未实现此两回调，为 NULL）：

```c
PluginHttpGetFunc  http_get  = params->http_get_func;   // 若 NULL，旧宿主不支持
PluginHttpPostFunc http_post = params->http_post_func;  // 若 NULL，旧宿主不支持
```

**参数**

- `url`：请求 URL，必须非空
- `data`：POST 请求 body（纯字符串），GET 请求忽略
- `headers_json`：请求头 JSON 对象字符串，键值均为 string，如 `{"User-Agent":"BotSDK/1.0","Content-Type":"application/json"}`；传 `{}` 表示无额外头

**返回值**（统一 JSON 字符串）

返回 JSON 对象，可直接用 nlohmann/json 解析：

```json
{
  "success":     true,    // bool，请求是否成功（网络层 + HTTP 层）
  "status_code": 200,     // int，HTTP 状态码（失败时为 0）
  "body":        "...",   // string，响应正文
  "error":       ""       // string，错误信息（成功时为空）
}
```

> 返回的 `const char*` 指向宿主内部静态 `std::string` 缓冲区，**插件端请勿释放**；若需要长期保存请自行拷贝。当参数非法（空 URL、headers_json 解析失败等），`success=false` 且 `error` 会给出提示。
>
> 若需兼容旧桌面版，调用前必须做空指针判断，否则旧宿主会因函数指针为 NULL 导致崩溃：
>
> ```c
> if (g_http_get) {
>     const char* resp = g_http_get("https://httpbin.org/get",
>         "{\"User-Agent\":\"MyPlugin/1.0\"}");
>     ...
> } else {
>     g_log("warn", "HTTP GET not supported by host (old desktop version?)");
> }
> ```

### 线程安全

- 桌面版：WebView2 的 COM 调用只能在主线程进行，跨线程 UI 更新通过 `PostMessage` + 自定义消息（`WM_UILOG` / `WM_UISTATUS` / `WM_UIPLUGINS`）派发到主线程
- 服务器版：同样通过 `PostMessage` + 自定义消息结构体（`UILogMsg` / `UIStatusMsg` / `UIPluginsMsg` 等）将工作线程的 UI 更新请求派发到主线程的 `WindowProc`，避免跨线程 GDI 操作
- 两版插件管理器内部均使用 `std::mutex` 保护插件列表
- 消息队列使用 `std::mutex + std::condition_variable` 实现线程安全的生产/消费
- `m_message_count` 等计数使用 `std::atomic` 保证原子性
- **插件开发者请注意**：`plugin_handle_message` 可能被多个工作线程**同时调用**，若插件内部有全局变量/共享状态，请自行加锁保护

### 界面主题自定义

- **桌面版**：直接编辑 `botpy_WebView2/ui/index.html` 中的 CSS，然后重新编译（HTML 会重新以资源形式嵌入 EXE）
- **服务器版**：编辑 `ServerVersion/Botpy_WindowEx/Botpy_WindowEx/MainWindow.h` 中的 `ThemeColors` 结构体各字段，修改后重新编译即可生效（无需额外资源文件）

## 📄 许可证

本项目仅供学习和个人使用。
