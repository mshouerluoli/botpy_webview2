#pragma once

#include <string>
#include <functional>
#include <atomic>
#include <thread>
#include <mutex>
#include <windows.h>
#include <winhttp.h>

#include "websocket.h"

class WebSocketClient {
public:
    using MessageHandler = std::function<void(const std::string&)>;

    WebSocketClient();
    ~WebSocketClient();

    void set_message_handler(MessageHandler handler);

    bool connect(const std::string& url);
    void close();

    bool send(const std::string& message);

    bool is_connected() const { return m_connected; }

private:
    bool _parse_url(const std::string& url, std::string& scheme_str, std::string& host_str, std::string& path_str, unsigned int& port);
    void _listener();

    HINTERNET m_hSession;
    WinHttpWebSocketClient::WebSocketClient* m_ws;
    bool m_use_tls;

    std::atomic<bool> m_connected{false};
    std::atomic<bool> m_running{false};
    std::thread m_listener_thread;
    std::mutex m_mutex;

    MessageHandler m_message_handler;
    std::string m_fragment_buffer;
};
