# 🐱 Miao Bot (botpy_WebView2)

基于 C++ 和 WebView2 开发的 QQ 机器人客户端，通过 WebSocket 连接 QQ 官方机器人网关，提供桌面 GUI 界面实时展示运行状态和日志，并内置**插件系统**支持动态加载第三方功能模块。

## ✨ 功能特性

- **桌面 GUI 界面**：使用 WebView2 渲染 HTML/CSS/JS 界面，采用毛玻璃质感 + 渐变光晕的现代暗色主题
- **WebSocket 通信**：基于 WinHttpWebSocket API 连接 QQ 机器人网关
- **自动重连**：连接断开后自动重新认证并重连，无需人工干预
- **心跳保活**：定时发送心跳包，检测连接状态，超时自动重连
- **消息处理**：支持私聊消息（C2C_MESSAGE_CREATE）和群@消息（GROUP_AT_MESSAGE_CREATE）
- **实时日志**：所有运行日志、状态变化、收发消息实时显示在 UI 中，自动换行、横向滚动条隐藏
- **资源嵌入**：HTML 界面以 RCDATA 资源形式嵌入 EXE，启动时自动释放，无需额外文件
- **⚡ 多线程消息处理**：基于消息队列 + 工作线程池的生产者-消费者模型，4 个工作线程并发处理消息，高并发下不阻塞 WebSocket 接收
- **🔌 插件系统**：支持动态加载 DLL 插件，提供独立的插件数据目录、优先级调度、消息拦截机制，可在 UI 中启用/停用/卸载/重载

## 🛠️ 技术栈

| 组件 | 技术 |
|------|------|
| 语言 | C++ (C++17) |
| UI | WebView2 + HTML/CSS/JS |
| WebSocket | WinHttpWebSocket API |
| HTTP | WinHTTP |
| JSON | nlohmann/json |
| 插件接口 | C ABI（DLL 导出函数） |
| 构建 | Visual Studio 2022 |
| 平台 | Windows (x64) |

## 📁 项目结构

```
botpy_WebView2/
├── botpy_WebView2.slnx              # 解决方案文件
├── Miao.py                          # Python 参考脚本
├── botpy_WebView2/                  # 主程序
│   ├── botpy_WebView2.vcxproj       # VS 项目文件
│   ├── botpy_WebView2.rc            # 资源文件（嵌入 HTML）
│   ├── resource.h                   # 资源头文件
│   ├── plugin_api.h                 # 🔌 插件 API 定义（插件与主程序共享）
│   ├── message_queue.h              # ⚡ 线程安全的消息队列（工作线程池用）
│   ├── MainWindow.h/cpp             # 主窗口及 WebView2 管理
│   ├── Miao.h/cpp                   # 机器人客户端核心逻辑 + 插件管理器
│   ├── webview_handlers.h/cpp       # WebView2 事件处理器
│   ├── json.hpp                     # nlohmann/json 库
│   ├── ui/
│   │   └── index.html               # 前端界面
│   ├── http/
│   │   ├── websocket_client.hpp/cpp # WebSocket 客户端
│   │   ├── winhttp_helper.hpp       # WinHTTP 辅助
│   │   ├── restclient.hpp           # REST API 客户端
│   │   └── textconv_helper.hpp      # 文本编码转换
│   └── websocket/
│       └── websocket.h/cpp          # WebSocket 底层封装
└── sdk/
    └── botpy_sdk/                   # 🔌 插件 SDK（示例插件工程）
        ├── botpy_sdk.slnx
        └── botpy_sdk/
            ├── dllmain.cpp          # 示例插件实现
            └── plugin_api.h         # 与主程序同源的 API 头文件
```

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
- WebView2 Runtime（Windows 10/11 通常已预装）
- Windows SDK

### 编译步骤

1. 用 Visual Studio 打开 `botpy_WebView2.slnx`
2. 选择 `Release` | `x64` 配置
3. 生成解决方案（`Ctrl+Shift+B`）

### 运行步骤

1. 将生成的 `botpy_WebView2.exe` 复制到任意目录
2. 在同目录下创建 `config.yaml` 并填入 AppID 和 Secret
3. 运行 `botpy_WebView2.exe`

程序启动后会自动：

- 从资源中释放 `ui/index.html` 到运行目录的 `ui/` 子目录
- 初始化 WebView2 并加载界面
- 读取 `config.yaml` 配置
- 创建 `plugins/` 与 `plugin_data/` 目录（不存在时）
- 加载 `plugins/` 下所有插件 DLL
- 连接 QQ 机器人网关并开始监听消息

## 🖥️ 界面说明

界面采用暗色毛玻璃风格，分为左侧信息栏和右侧标签页区。

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
├── botpy_WebView2.exe
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

在 [plugin_api.h](file:///D:/Vsyuanma/botpy_WebView2/botpy_WebView2/plugin_api.h) 中定义。**必选**导出：

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
    const char* openid;       // 私聊对方 openid
    const char* group_openid; // 群 openid
} PluginMessage;

// 初始化时主程序传入的参数
typedef struct {
    PluginLogFunc log_func;        // 日志输出函数
    PluginSendMessageFunc send_msg_func; // 发送消息函数
    const char* appid;             // 当前机器人 AppID
    const char* data_path;         // 插件专属数据目录路径
} PluginInitParams;
```

### 优先级与消息拦截

- 插件按 `plugin_get_priority()` 返回值**升序**处理（数值越小越先收到消息）
- 高优先级插件若在 `plugin_handle_message` 中 `return 1`，则后续低优先级插件**不会**收到该消息
- 返回 0 表示放行，消息继续传递给下一个插件
- 已停用（`enabled = false`）的插件不参与消息分发

### 最小示例

参见 [sdk/botpy_sdk/botpy_sdk/dllmain.cpp](file:///D:/Vsyuanma/botpy_WebView2/sdk/botpy_sdk/botpy_sdk/dllmain.cpp)。核心骨架：

```c
#include "plugin_api.h"
#include <string>

static PluginLogFunc g_log = nullptr;
static PluginSendMessageFunc g_send_msg = nullptr;
static std::string data_dir;

extern "C" __declspec(dllexport) int plugin_init(const PluginInitParams* params) {
    g_log = params->log_func;
    g_send_msg = params->send_msg_func;
    data_dir = params->data_path;        // 自己的数据目录
    if (g_log) g_log("info", "MyPlugin loaded");
    return 0;                            // 0 = 加载成功
}

extern "C" __declspec(dllexport) int plugin_handle_message(const PluginMessage* msg) {
    if (!msg) return 0;
    if (msg->is_group && msg->group_openid) {
        g_send_msg(msg->group_openid, "Hello from plugin!", 1);
    } else if (!msg->is_group && msg->openid) {
        g_send_msg(msg->openid, "Hello from plugin!", 0);
    }
    return 1;                            // 1 = 拦截，不再向下传递
}

extern "C" __declspec(dllexport) void plugin_shutdown() {
    if (g_log) g_log("info", "MyPlugin unloaded");
}

extern "C" __declspec(dllexport) const char* plugin_get_name()        { return "MyPlugin"; }
extern "C" __declspec(dllexport) int          plugin_get_priority()   { return 100; }
extern "C" __declspec(dllexport) const char* plugin_get_author()      { return "Miaopasi"; }
extern "C" __declspec(dllexport) const char* plugin_get_description() { return "示例插件"; }
```

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

插件调用 `send_msg_func(target_id, content, is_group)`：

- 私聊：POST `/v2/users/{openid}/messages`
- 群聊：POST `/v2/groups/{group_openid}/messages`

### 线程安全

- WebView2 的 COM 调用只能在主线程进行，跨线程 UI 更新通过 `PostMessage` + 自定义消息（`WM_UILOG` / `WM_UISTATUS` / `WM_UIPLUGINS`）派发到主线程
- 插件管理器内部使用 `std::mutex` 保护插件列表
- 消息队列使用 `std::mutex + std::condition_variable` 实现线程安全的生产/消费
- `m_message_count` 等计数使用 `std::atomic` 保证原子性
- **插件开发者请注意**：`plugin_handle_message` 可能被多个工作线程**同时调用**，若插件内部有全局变量/共享状态，请自行加锁保护

## 📄 许可证

本项目仅供学习和个人使用。
