#include "Miao.h"
#include "MainWindow.h"
#include "json.hpp"
#include <iostream>
#include <fstream>
#include <regex>
#include <chrono>
#include <sstream>
#include <cstring>
#include <locale>
#include <windows.h>

using json = nlohmann::json;

static MainWindow* g_main_window = nullptr;

static void ui_log(const std::string& level, const std::string& msg) {
    if (g_main_window) {
        g_main_window->PostLog(level, msg);
    }
}

static std::string get_log_path() {
    wchar_t exe_path[MAX_PATH];
    GetModuleFileNameW(NULL, exe_path, MAX_PATH);
    std::wstring exe_dir_w = exe_path;
    size_t pos = exe_dir_w.find_last_of(L"\\/");
    if (pos != std::wstring::npos) {
        exe_dir_w = exe_dir_w.substr(0, pos);
    }
    std::string exe_dir(exe_dir_w.begin(), exe_dir_w.end());
    return exe_dir + "/log.txt";
}

void MyClient::log(const std::string& level, const std::string& msg) {
    auto now = std::chrono::system_clock::now();
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);
    char time_str[32];
    std::tm time_info;
    localtime_s(&time_info, &now_time);
    std::strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", &time_info);
    
    std::string log_line = "[" + std::string(time_str) + "] [" + level + "] " + msg + "\n";
    
    std::ofstream log_file(get_log_path(), std::ios::app);
    if (log_file.is_open()) {
        log_file << log_line;
        log_file.flush();
    }
    
    if (on_log) {
        on_log(level, msg);
    }
}

#define _USE_WIHTTP_INTERFACE
#include "http/restclient.hpp"
#include "http/websocket_client.hpp"

uint32_t Intents::to_bits() const {
    uint32_t bits = 0;
    if (guilds) bits |= 1 << 0;
    if (guild_members) bits |= 1 << 1;
    if (guild_messages) bits |= 1 << 9;
    if (guild_message_reactions) bits |= 1 << 10;
    if (direct_message) bits |= 1 << 12;
    if (open_forum_event) bits |= 1 << 18;
    if (audio_or_live_channel_member) bits |= 1 << 19;
    if (public_messages) bits |= 1 << 25;
    if (interaction) bits |= 1 << 26;
    if (message_audit) bits |= 1 << 27;
    if (forums) bits |= 1 << 28;
    if (audio_action) bits |= 1 << 29;
    if (public_guild_messages) bits |= 1 << 30;
    return bits;
}

bool Config::load_from_file(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        ui_log("error", "Config file not found: " + path);
        return false;
    }
    
    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    
    std::smatch match;
    std::regex appid_regex(R"(appid\s*:\s*["']([^"']+)["'])");
    std::regex secret_regex(R"(secret\s*:\s*["']([^"']+)["'])");
    
    if (std::regex_search(content, match, appid_regex)) {
        appid = match[1].str();
    }
    if (std::regex_search(content, match, secret_regex)) {
        secret = match[1].str();
    }
    
    return !appid.empty() && !secret.empty();
}

MyClient::MyClient() 
    : m_running(false), m_websocket_connected(false), 
      m_heartbeat_interval(0), m_sequence(0), m_message_count(0), m_heartbeat_count(0),
      m_last_message_ts(0), m_websocket(nullptr) {}

MyClient::~MyClient() {
    stop();
}

bool MyClient::_authenticate() {
    log("info", "Authenticating with appid: " + m_appid);
    
    std::string body = R"({"appId":")" + m_appid + R"(","clientSecret":")" + m_secret + R"("})";
    RestClient::Response response = RestClient::post("https://api.bot.qq.com/app/getAppAccessToken", "application/json", body);
    
    if (response.code == 0 || response.body.empty()) {
        log("error", "Authentication request failed");
        return false;
    }
    
    log("info", "Auth response: " + response.body);
    
    std::string token;
    try {
        json j = json::parse(response.body);
        if (j.contains("access_token")) {
            token = j["access_token"].get<std::string>();
        }
    } catch (const std::exception& e) {
        log("error", std::string("Auth JSON parse error: ") + e.what());
    }
    if (token.empty()) {
        log("error", "Failed to extract access_token");
        return false;
    }
    
    m_token = token;
    log("success", "Authentication successful");
    return true;
}

bool MyClient::_get_gateway_url() {
    log("info", "Getting gateway URL...");
    
    RestClient::Request request;
    request.headers["Authorization"] = "QQBot " + m_token;
    
    RestClient::Response response = RestClient::get("https://api.bot.qq.com/gateway/bot", &request);
    
    if (response.code == 0 || response.body.empty()) {
        log("error", "Failed to get gateway URL");
        return false;
    }
    
    log("info", "Gateway response: " + response.body);
    
    std::string gateway;
    try {
        json j = json::parse(response.body);
        if (j.contains("url")) {
            gateway = j["url"].get<std::string>();
        }
    } catch (const std::exception& e) {
        log("error", std::string("Gateway JSON parse error: ") + e.what());
    }
    if (gateway.empty()) {
        log("error", "Failed to extract gateway URL");
        return false;
    }
    
    m_gateway_url = gateway;
    log("info", "Gateway URL: " + m_gateway_url);
    return true;
}

bool MyClient::_connect_websocket() {
    log("info", "Connecting to WebSocket: " + m_gateway_url);
    
    WebSocketClient* new_ws = new WebSocketClient();
    new_ws->set_message_handler([this](const std::string& msg) {
        _handle_event(msg);
    });
    
    if (!new_ws->connect(m_gateway_url)) {
        delete new_ws;
        return false;
    }
    
    {
        std::lock_guard<std::mutex> lock(m_websocket_mutex);
        m_websocket = new_ws;
    }
    m_websocket_connected = true;
    log("success", "WebSocket connected successfully");
    return true;
}

void MyClient::_send_identify() {
    if (!m_websocket_connected) return;

    uint32_t intents = (1u << 25) | (1u << 12);

    std::string json = R"({"op":2,"d":{"token":"QQBot )" + m_token +
        R"(","intents":)" + std::to_string(intents) +
        R"(,"properties":{"os":"windows","browser":"MiaoBot","device":"MiaoBot"}}})";

    bool send_result;
    {
        std::lock_guard<std::mutex> lock(m_websocket_mutex);
        if (!m_websocket) {
            send_result = false;
        } else {
            send_result = m_websocket->send(json);
        }
    }

    if (!send_result) {
        log("error", "Failed to send identify");
    } else {
        log("info", "Identify sent");
    }
}

void MyClient::_send_heartbeat() {
    while (m_running) {
        if (m_websocket_connected && m_websocket) {
            if (m_heartbeat_interval > 0) {
                std::string json = R"({"op":1,"d":)" + std::to_string(m_sequence) + R"(})";

                bool send_result;
                {
                    std::lock_guard<std::mutex> lock(m_websocket_mutex);
                    if (!m_websocket) {
                        send_result = false;
                    } else {
                        send_result = m_websocket->send(json);
                    }
                }

                if (!send_result) {
                    log("error", "Failed to send heartbeat");
                    m_websocket_connected = false;
                    if (on_status) on_status("connecting", "心跳失败，重启中...");
                    stop();
                    if (on_restart) on_restart();
                    return;
                } else {
                    m_heartbeat_count++;
                    if (on_info) on_info("heartbeatCount", std::to_string(m_heartbeat_count));
                }

                std::this_thread::sleep_for(std::chrono::milliseconds(m_heartbeat_interval));
            } else {
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        } else {
            if (m_running) {
                log("warn", "WebSocket disconnected");
                if (on_status) on_status("connecting", "断开连接，重启中...");
                stop();
                if (on_restart) on_restart();
                return;
            }
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
    }
}

bool MyClient::_reconnect() {
    {
        std::lock_guard<std::mutex> lock(m_websocket_mutex);
        if (m_websocket) {
            m_websocket->close();
            delete m_websocket;
            m_websocket = nullptr;
        }
    }
    m_websocket_connected = false;
    m_sequence = 0;
    m_session_id.clear();

    if (!_authenticate()) {
        log("error", "Reconnect: authentication failed");
        return false;
    }

    if (!_get_gateway_url()) {
        log("error", "Reconnect: failed to get gateway URL");
        return false;
    }

    if (!_connect_websocket()) {
        log("error", "Reconnect: WebSocket connection failed");
        return false;
    }

    _send_identify();
    log("success", "Reconnect successful");
    return true;
}

void MyClient::_handle_event(const std::string& event_json) {
    uint64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
    m_last_message_ts.store(now);

    try {
        json j = json::parse(event_json);

        if (j.contains("s") && j["s"].is_number()) {
            m_sequence = j["s"].get<int>();
        }

        if (!j.contains("op") || !j["op"].is_number()) return;
        int op = j["op"].get<int>();

        if (op == 0) {
            std::string t;
            if (j.contains("t") && j["t"].is_string()) {
                t = j["t"].get<std::string>();
            }

            json d;
            if (j.contains("d") && j["d"].is_object()) {
                d = j["d"];
            }

            if (t == "READY") {
                if (d.contains("session_id") && d["session_id"].is_string()) {
                    m_session_id = d["session_id"].get<std::string>();
                }
                if (d.contains("user") && d["user"].is_object()) {
                    json user = d["user"];
                    if (user.contains("username") && user["username"].is_string()) {
                        m_nickname = user["username"].get<std::string>();
                    }
                }

                on_ready();
            } else if (t == "C2C_MESSAGE_CREATE") {
                C2CMessage cmsg;
                cmsg.is_group = false;
                if (d.contains("id") && d["id"].is_string()) {
                    cmsg.id = d["id"].get<std::string>();
                }
                if (d.contains("content") && d["content"].is_string()) {
                    cmsg.content = d["content"].get<std::string>();
                }
                if (d.contains("author") && d["author"].is_object()) {
                    json author = d["author"];
                    if (author.contains("user_openid") && author["user_openid"].is_string()) {
                        cmsg.openid = author["user_openid"].get<std::string>();
                    }
                }
                cmsg.sender_id = cmsg.openid;

                on_c2c_message_create(cmsg);
            } else if (t == "GROUP_AT_MESSAGE_CREATE") {
                GroupMessage gmsg;
                gmsg.is_group = true;
                if (d.contains("id") && d["id"].is_string()) {
                    gmsg.id = d["id"].get<std::string>();
                }
                if (d.contains("content") && d["content"].is_string()) {
                    gmsg.content = d["content"].get<std::string>();
                }
                if (d.contains("group_openid") && d["group_openid"].is_string()) {
                    gmsg.group_openid = d["group_openid"].get<std::string>();
                }
                if (d.contains("author") && d["author"].is_object()) {
                    json author = d["author"];
                    if (author.contains("member_openid") && author["member_openid"].is_string()) {
                        gmsg.sender_id = author["member_openid"].get<std::string>();
                    }
                }
                on_group_at_message_create(gmsg);
            }
        } else if (op == 10) {
            json d = j["d"];
            if (d.contains("heartbeat_interval") && d["heartbeat_interval"].is_number()) {
                m_heartbeat_interval = d["heartbeat_interval"].get<int>();
            }
            log("info", "Heartbeat interval: " + std::to_string(m_heartbeat_interval) + "ms");
            _send_identify();
        } else if (op == 11) {
            log("info", "Heartbeat ACK received");
        }
    } catch (const std::exception& e) {
        log("error", std::string("JSON parse error: ") + e.what());
    }
}

std::string MyClient::_build_msg_json(const std::string& content, const std::string& msg_id) {
    json j;
    j["content"] = content;
    j["msg_type"] = 0;
    if (!msg_id.empty()) {
        j["msg_id"] = msg_id;
    }
    return j.dump();
}

bool MyClient::_send_c2c_message(const std::string& openid, const std::string& content, const std::string& msg_id) {
    if (m_token.empty() || openid.empty()) return false;

    RestClient::Request request;
    request.headers["Authorization"] = "QQBot " + m_token;
    request.headers["Content-Type"] = "application/json";

    std::string url = "https://api.sgroup.qq.com/v2/users/" + openid + "/messages";
    std::string body = _build_msg_json(content, msg_id);

    RestClient::Response response = RestClient::post(url, "application/json", body, &request);

    if (response.code == 0 || response.body.empty()) {
        log("error", "Failed to send C2C message, code: " + std::to_string(response.code));
        return false;
    }
    return response.code >= 200 && response.code < 300;
}

bool MyClient::_send_group_message(const std::string& group_openid, const std::string& content, const std::string& msg_id) {
    if (m_token.empty() || group_openid.empty()) return false;

    RestClient::Request request;
    request.headers["Authorization"] = "QQBot " + m_token;
    request.headers["Content-Type"] = "application/json";

    std::string url = "https://api.sgroup.qq.com/v2/groups/" + group_openid + "/messages";
    std::string body = _build_msg_json(content, msg_id);

    RestClient::Response response = RestClient::post(url, "application/json", body, &request);

    if (response.code == 0 || response.body.empty()) {
        log("error", "Failed to send group message, code: " + std::to_string(response.code));
        return false;
    }

    return response.code >= 200 && response.code < 300;
}

void MyClient::_handle_common_commands(const Message& message, bool message_isgroup) {
    m_message_count++;
    if (on_info) on_info("msgCount", std::to_string(m_message_count));

    std::string reply = "Received: " + message.content;

    if (message_isgroup) {
        if (!message.group_openid.empty()) {
            _send_group_message(message.group_openid, reply, message.id);
        }
    } else {
        if (!message.openid.empty()) {
            _send_c2c_message(message.openid, reply, message.id);
        }
    }
}

void MyClient::on_ready() {
    log("success", "Robot on_ready!");
    if (on_status) on_status("online", "在线");
    if (on_info) on_info("sessionId", m_session_id);
    if (on_info && !m_nickname.empty()) on_info("nickname", m_nickname);
}

void MyClient::on_c2c_message_create(const C2CMessage& message) {
    if (on_message) on_message(false, message.content);
    _handle_common_commands(message, false);
}

void MyClient::on_group_at_message_create(const GroupMessage& message) {
    if (on_message) on_message(true, message.content);
    _handle_common_commands(message, true);
}

void MyClient::run(const std::string& appid, const std::string& secret) {
    m_appid = appid;
    m_secret = secret;
    m_running = true;
    m_websocket_connected = false;
    m_heartbeat_interval = 0;
    m_sequence = 0;
    m_session_id.clear();
    m_message_count = 0;
    m_heartbeat_count = 0;

    if (on_info) on_info("appId", appid);
    if (on_info) on_info("msgCount", "0");
    if (on_info) on_info("heartbeatCount", "0");

    if (on_status) on_status("connecting", "connecting...");

    log("info", "----------------------------------------");
    log("info", "Starting client...");

    if (!_authenticate()) {
        if (on_status) on_status("offline", "auth failed");
        return;
    }

    if (!_get_gateway_url()) {
        log("error", "Failed to get gateway URL");
        if (on_status) on_status("offline", "gateway failed");
        return;
    }

    if (!_connect_websocket()) {
        log("error", "WebSocket connection failed");
        if (on_status) on_status("offline", "websocket failed");
        return;
    }

    if (m_heartbeat_thread.joinable()) {
        m_heartbeat_thread.detach();
    }
    m_heartbeat_thread = std::thread(&MyClient::_send_heartbeat, this);
    
    log("info", "Client started");
}

void MyClient::stop() {
    if (!m_running) return;
    
    m_running = false;
    m_websocket_connected = false;
    
    {
        std::lock_guard<std::mutex> lock(m_websocket_mutex);
        if (m_websocket) {
            m_websocket->close();
            delete m_websocket;
            m_websocket = nullptr;
        }
    }
    
    if (m_heartbeat_thread.joinable()) {
        m_heartbeat_thread.detach();
    }
    
    log("info", "Client stopped");
    if (on_status) on_status("offline", "已断开");
}


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    wchar_t exe_path[MAX_PATH];
    GetModuleFileNameW(NULL, exe_path, MAX_PATH);
    std::wstring exe_dir_w = exe_path;
    size_t pos = exe_dir_w.find_last_of(L"\\/");
    if (pos != std::wstring::npos) {
        exe_dir_w = exe_dir_w.substr(0, pos);
    }

    std::string exe_dir(exe_dir_w.begin(), exe_dir_w.end());
    std::string config_path = exe_dir + "\\config.yaml";

    MainWindow mainWin;
    g_main_window = &mainWin;

    if (!mainWin.Create(hInstance, nCmdShow)) {
        MessageBoxA(NULL, "Failed to create main window", "Miao Bot Error", MB_ICONERROR);
        return 1;
    }

    Config config;
    if (!config.load_from_file(config_path)) {
        MessageBoxA(NULL, ("Config file not found: " + config_path).c_str(), "Miao Bot Error", MB_ICONERROR);
        return 1;
    }

    if (config.appid.empty() || config.secret.empty()) {
        MessageBoxA(NULL, "appid or secret is empty in config.yaml", "Miao Bot Error", MB_ICONERROR);
        return 1;
    }

    MyClient client;

    client.on_log = [&mainWin](const std::string& level, const std::string& msg) {
        mainWin.PostLog(level, msg);
    };
    client.on_status = [&mainWin](const std::string& status, const std::string& text) {
        mainWin.PostStatus(status, text);
    };
    client.on_info = [&mainWin](const std::string& id, const std::string& value) {
        mainWin.PostInfo(id, value);
    };
    client.on_message = [&mainWin](bool is_group, const std::string& content) {
        mainWin.PostMessageEvent(is_group, content);
    };
    client.on_restart = [&client, &config]() {
        std::this_thread::sleep_for(std::chrono::seconds(3));
        try {
            client.run(config.appid, config.secret);
        } catch (const std::exception& ) {
        } catch (...) {
        }
    };

    mainWin.SetCommandHandler([&client](const std::string& cmd) {
        if (cmd == "stop") {
            client.stop();
        }
    });

    mainWin.SetReadyHandler([&client, &config]() {
        std::thread bot_thread([&client, &config]() {
            try {
                client.run(config.appid, config.secret);
            } catch (const std::exception& ) {

            } catch (...) {

            }
        });
        bot_thread.detach();
    });

    mainWin.RunMessageLoop();

    client.stop();
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    return 0;
}
