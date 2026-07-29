#include "websocket_client.hpp"
#include <iostream>
#include <cctype>
#include <string>
#include <vector>

#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "Crypt32.lib")

WebSocketClient::WebSocketClient() : m_hSession(NULL), m_ws(NULL), m_use_tls(false) {}

WebSocketClient::~WebSocketClient() {
    close();
}

void WebSocketClient::set_message_handler(MessageHandler handler) {
    m_message_handler = handler;
}

bool WebSocketClient::_parse_url(const std::string& url, std::string& scheme_str, std::string& host_str, std::string& path_str, unsigned int& port) {
    size_t scheme_end = url.find("://");
    if (scheme_end == std::string::npos) return false;

    scheme_str = url.substr(0, scheme_end);
    for (size_t i = 0; i < scheme_str.size(); ++i) {
        scheme_str[i] = (char)tolower((unsigned char)scheme_str[i]);
    }

    if (scheme_str != "ws" && scheme_str != "wss") return false;

    std::string rest = url.substr(scheme_end + 3);

    size_t path_start = rest.find('/');
    std::string host_port_str;
    if (path_start != std::string::npos) {
        host_port_str = rest.substr(0, path_start);
        path_str = rest.substr(path_start);
    } else {
        host_port_str = rest;
        path_str = "/";
    }

    size_t colon_pos = host_port_str.find(':');
    if (colon_pos != std::string::npos) {
        host_str = host_port_str.substr(0, colon_pos);
        port = (unsigned int)std::stoi(host_port_str.substr(colon_pos + 1));
    } else {
        host_str = host_port_str;
        port = scheme_str == "wss" ? 443 : 80;
    }

    return true;
}

// Helper: convert std::string (ANSI/UTF-8) to std::wstring
static std::wstring s2w(const std::string& s) {
    if (s.empty()) return std::wstring();
    int len = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), NULL, 0);
    std::wstring w(len, 0);
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), &w[0], len);
    return w;
}

bool WebSocketClient::connect(const std::string& url) {
    std::string scheme_str, host_str, path_str;
    unsigned int port;

    if (!_parse_url(url, scheme_str, host_str, path_str, port)) {
        return false;
    }

    m_use_tls = (scheme_str == "wss");

    // Create a WinHTTP session
    m_hSession = WinHttpOpen(L"MiaoBot/1.0",
                             WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
                             WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!m_hSession) {
        return false;
    }

    // Create the WebSocket client wrapper
    m_ws = new WinHttpWebSocketClient::WebSocketClient();
    DWORD err = m_ws->Initialize(m_hSession, NULL);
    if (err != ERROR_SUCCESS) {
        delete m_ws;
        m_ws = NULL;
        WinHttpCloseHandle(m_hSession);
        m_hSession = NULL;
        return false;
    }

    // Build the full URL to pass to Connect (it uses WinHttpCrackUrl)
    // WinHttpCrackUrl does not recognize ws/wss schemes, so convert to http/https.
    std::string full_url = url;
    if (scheme_str == "wss") {
        full_url = "https://" + url.substr(6);
    } else if (scheme_str == "ws") {
        full_url = "http://" + url.substr(5);
    }
    std::wstring wurl = s2w(full_url);

    DWORD flags = m_use_tls ? WEBSOCKET_SECURE_CONNECTION : 0;

    err = m_ws->Connect((WCHAR*)wurl.c_str(), flags, NULL);
    if (err != ERROR_SUCCESS) {
        m_ws->Free();
        delete m_ws;
        m_ws = NULL;
        WinHttpCloseHandle(m_hSession);
        m_hSession = NULL;
        return false;
    }

    m_connected = true;
    m_running = true;

    m_listener_thread = std::thread(&WebSocketClient::_listener, this);

    return true;
}

void WebSocketClient::close() {
    m_running = false;
    m_connected = false;

    if (m_ws) {
        m_ws->Close(WINHTTP_WEB_SOCKET_SUCCESS_CLOSE_STATUS, NULL);
        m_ws->Free();
        delete m_ws;
        m_ws = NULL;
    }

    if (m_hSession) {
        WinHttpCloseHandle(m_hSession);
        m_hSession = NULL;
    }

    if (m_listener_thread.joinable()) {
        m_listener_thread.join();
    }

}

bool WebSocketClient::send(const std::string& message) {
    if (!m_connected || !m_ws) return false;

    DWORD err = m_ws->Send(WINHTTP_WEB_SOCKET_UTF8_MESSAGE_BUFFER_TYPE,
                           (void*)message.c_str(), (DWORD)message.size());
    if (err != ERROR_SUCCESS) {
        return false;
    }

    return true;
}

void WebSocketClient::_listener() {
    char buffer[8192];

    while (m_running && m_ws) {
        DWORD bytesReceived = 0;
        WINHTTP_WEB_SOCKET_BUFFER_TYPE bufferType = WINHTTP_WEB_SOCKET_CLOSE_BUFFER_TYPE;

        DWORD err = m_ws->Receive(buffer, sizeof(buffer) - 1, &bytesReceived, &bufferType);

        if (err == ERROR_SUCCESS) {
            if (bufferType == WINHTTP_WEB_SOCKET_UTF8_FRAGMENT_BUFFER_TYPE ||
                bufferType == WINHTTP_WEB_SOCKET_BINARY_FRAGMENT_BUFFER_TYPE) {
                // Fragment; keep accumulating (simple impl: just pass through fragment)
                if (bytesReceived > 0 && m_message_handler) {
                    std::string payload(buffer, bytesReceived);
                    m_message_handler(payload);
                }
            } else if (bufferType == WINHTTP_WEB_SOCKET_UTF8_MESSAGE_BUFFER_TYPE) {
                if (bytesReceived > 0) {
                    buffer[bytesReceived] = '\0';
                    std::string payload(buffer, bytesReceived);

                    if (m_message_handler) {
                        m_message_handler(payload);
                    }
                }
            } else if (bufferType == WINHTTP_WEB_SOCKET_BINARY_MESSAGE_BUFFER_TYPE) {
                if (bytesReceived > 0) {
                    std::string payload(buffer, bytesReceived);
                    if (m_message_handler) {
                        m_message_handler(payload);
                    }
                }
            } else if (bufferType == WINHTTP_WEB_SOCKET_CLOSE_BUFFER_TYPE) {
                break;
            }
        } else {
            break;
        }
    }

    m_connected = false;
}
