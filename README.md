# 🐱 Miao Bot (botpy_WebView2)

基于 C++ 和 WebView2 开发的 QQ 机器人客户端，通过 WebSocket 连接 QQ 官方机器人网关，提供桌面 GUI 界面实时展示运行状态和日志。

## ✨ 功能特性

- **桌面 GUI 界面**：使用 WebView2 渲染 HTML/CSS/JS 界面，无需依赖额外的 UI 框架
- **WebSocket 通信**：基于 WinHttpWebSocket API 连接 QQ 机器人网关
- **自动重连**：连接断开后自动重新认证并重连，无需人工干预
- **心跳保活**：定时发送心跳包，检测连接状态，超时自动重连
- **消息处理**：支持私聊消息（C2C_MESSAGE_CREATE）和群@消息（GROUP_AT_MESSAGE_CREATE）
- **实时日志**：所有运行日志、状态变化、收发消息实时显示在 UI 中
- **资源嵌入**：HTML 界面以 RCDATA 资源形式嵌入 EXE，启动时自动释放，无需额外文件

## 🛠️ 技术栈

| 组件 | 技术 |
|------|------|
| 语言 | C++ (C++17) |
| UI | WebView2 + HTML/CSS/JS |
| WebSocket | WinHttpWebSocket API |
| HTTP | WinHTTP |
| JSON | nlohmann/json |
| 构建 | Visual Studio 2022 |
| 平台 | Windows (x64) |

## 📁 项目结构

```
botpy_WebView2/
├── botpy_WebView2.slnx          # 解决方案文件
├── Miao.py                      # Python 参考脚本
└── botpy_WebView2/
    ├── botpy_WebView2.vcxproj   # VS 项目文件
    ├── botpy_WebView2.rc        # 资源文件（嵌入 HTML）
    ├── resource.h               # 资源头文件
    ├── MainWindow.h/cpp         # 主窗口及 WebView2 管理
    ├── Miao.h/cpp               # 机器人客户端核心逻辑
    ├── webview_handlers.h/cpp   # WebView2 事件处理器
    ├── json.hpp                 # nlohmann/json 库
    ├── ui/
    │   └── index.html           # 前端界面
    ├── http/
    │   ├── websocket_client.hpp/cpp  # WebSocket 客户端
    │   ├── winhttp_helper.hpp        # WinHTTP 辅助
    │   ├── restclient.hpp            # REST API 客户端
    │   └── textconv_helper.hpp       # 文本编码转换
    └── websocket/
        ├── websocket.h/cpp      # WebSocket 底层封装
```

## 📋 配置说明

在 EXE 同目录下创建 `config.yaml` 文件：

```yaml
appid: "你的机器人AppID"
secret: "你的机器人Secret"
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
- 从资源中释放 `ui/index.html` 到运行目录
- 初始化 WebView2 并加载界面
- 读取 `config.yaml` 配置
- 连接 QQ 机器人网关并开始监听消息

## 🖥️ 界面说明

界面分为左侧信息栏和右侧日志区：

**机器人信息**
- AppID：机器人应用 ID
- 昵称：机器人账号昵称（连接成功后显示）
- 会话 ID：WebSocket 会话 ID
- 消息数：累计收到的消息数量
- 心跳数：累计发送的心跳次数

**运行日志**
- 实时显示连接状态、认证过程、心跳、消息收发等日志
- 收到消息时显示 `[群]` 或 `[私聊]` 前缀及消息内容

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

### 消息回复

收到消息后自动回复 `Received: [原消息内容]`：
- 私聊：POST `/v2/users/{openid}/messages`
- 群聊：POST `/v2/groups/{group_openid}/messages`

## 📄 许可证

本项目仅供学习和个人使用。
