#include "framework.h"


#include "plugin_api.h"
#include "json.hpp"
#include <cstring>
#include <cstdio>


using json = nlohmann::json;

// Unified context: holds function pointers (via PluginInitParams)
// plus owned string copies for appid / data_path, whose source
// pointers may be transient after plugin_init returns.
struct PluginContext {
    PluginInitParams api;
    std::string appid;
    std::string data_dir;
};
static PluginContext g_ctx = {};

// Parse the unified HTTP JSON response returned by the host
// Format: {"success":bool, "status_code":int, "body":"...", "error":"..."}
static bool parse_http_response(const char* resp, bool& success, int& status_code,
    std::string& body, std::string& error)
{
    success = false;
    status_code = 0;
    body.clear();
    error.clear();
    if (!resp || !*resp) {
        error = "empty response";
        return false;
    }
    try {
        json j = json::parse(resp);
        if (j.contains("success")) success = j["success"].get<bool>();
        if (j.contains("status_code")) status_code = j["status_code"].get<int>();
        if (j.contains("body")) body = j["body"].get<std::string>();
        if (j.contains("error")) error = j["error"].get<std::string>();
        return true;
    } catch (const std::exception& e) {
        error = std::string("parse error: ") + e.what();
        return false;
    }
}

extern "C" __declspec(dllexport) int plugin_init(const PluginInitParams* params) {
    if (!params) return -1;

    // Copy function pointers directly from params struct
    g_ctx.api = *params;

    // Take ownership of string data (original pointers may be transient)
    if (params->appid) g_ctx.appid = params->appid;
    if (params->data_path) g_ctx.data_dir = params->data_path;

    // Re-point struct pointers to our owned copies
    g_ctx.api.appid = g_ctx.appid.c_str();
    g_ctx.api.data_path = g_ctx.data_dir.c_str();

    if (g_ctx.api.log_func) g_ctx.api.log_func("info", std::string(Plugin::name + " loaded").c_str());
    if (g_ctx.api.log_func) {
        std::string full_msg = "data_dir: " + g_ctx.data_dir;
        g_ctx.api.log_func("info", full_msg.c_str());
    }
    if (g_ctx.api.log_func) {
        if (g_ctx.api.http_get_func) g_ctx.api.log_func("info", "HTTP GET API available");
        else g_ctx.api.log_func("warn", "HTTP GET API NOT available (old host?)");
        if (g_ctx.api.http_post_func) g_ctx.api.log_func("info", "HTTP POST API available");
        else g_ctx.api.log_func("warn", "HTTP POST API NOT available (old host?)");
    }
    return 0;
}
extern "C" __declspec(dllexport) void plugin_shutdown() {
    if (g_ctx.api.log_func) g_ctx.api.log_func("info", std::string(Plugin::name + " unloaded").c_str());
}

extern "C" __declspec(dllexport) const char* plugin_get_name() {
    return Plugin::name.c_str();
}
extern "C" __declspec(dllexport) const char* plugin_get_author(void) {
    return Plugin::author.c_str();
}

extern "C" __declspec(dllexport) const char* plugin_get_description(void) {
    return Plugin::description.c_str();
}
extern "C" __declspec(dllexport) int plugin_get_priority() {
    return Plugin::priority;
}

void send_msg(const PluginMessage* msg, const char* Text ,int msg_type = 0, const char* media = nullptr) {
    if (g_ctx.api.send_msg_func) {
        if (msg->is_group && msg->group_openid) {
            g_ctx.api.send_msg_func(msg->group_openid, Text, 1, msg->id, msg_type, media);
        }
        else if (!msg->is_group && msg->openid) {
            g_ctx.api.send_msg_func(msg->openid, Text, 0, msg->id, msg_type, media);
        }

    }

}

void sendGroupImageMessage(const PluginMessage* msg, const char* imageUrl, const char* successText, const char* failText) {
    if (!msg || !g_ctx.api.post_file_func) return;

    const char* media = nullptr;
    if (msg->is_group) {
        media = g_ctx.api.post_file_func(msg->group_openid, imageUrl, 1, 1, 0);
    }
    else {
        media = g_ctx.api.post_file_func(msg->openid, imageUrl, 1, 0, 0);
    }
    

    if (media) {
        send_msg(msg, successText, 7, media);
    }
    else {
        send_msg(msg, failText, 0, media);
    }
}

// Example: trigger HTTP GET when user sends "/httpget"
static void demo_http_get(const PluginMessage* msg) {
    if (!g_ctx.api.http_get_func) {
        send_msg(msg, "[SDK] HTTP GET API is not available. Please update the host program.");
        return;
    }
    if (g_ctx.api.log_func) g_ctx.api.log_func("info", "Running HTTP GET demo...");

    const char* url = "https://httpbin.org/get";

    // Build headers JSON using nlohmann/json (type-safe, escapes automatically)
    json headers = {
        {"User-Agent", "BotpySDK/1.0"},
        {"X-Custom-Header", "hello"}
    };
    std::string headers_json_str = headers.dump();

    const char* resp = g_ctx.api.http_get_func(url, headers_json_str.c_str());
    if (!resp) {
        send_msg(msg, "[SDK] HTTP GET returned NULL");
        return;
    }

    bool success = false;
    int status_code = 0;
    std::string body;
    std::string error;
    parse_http_response(resp, success, status_code, body, error);

    char reply[2048];
    snprintf(reply, sizeof(reply),
        "[SDK HTTP GET]\n"
        "  success: %s\n"
        "  status:  %d\n"
        "  error:   %s\n"
        "  body(len): %zu",
        success ? "true" : "false", status_code,
        error.empty() ? "(none)" : error.c_str(),
        body.size());
    send_msg(msg, reply);

    if (g_ctx.api.log_func) {
        char logbuf[1024];
        snprintf(logbuf, sizeof(logbuf), "HTTP GET result: success=%d status=%d body_len=%zu",
            success ? 1 : 0, status_code, body.size());
        g_ctx.api.log_func("info", logbuf);
    }
}

// Example: trigger HTTP POST when user sends "/httppost"
static void demo_http_post(const PluginMessage* msg) {
    if (!g_ctx.api.http_post_func) {
        send_msg(msg, "[SDK] HTTP POST API is not available. Please update the host program.");
        return;
    }
    if (g_ctx.api.log_func) g_ctx.api.log_func("info", "Running HTTP POST demo...");

    const char* url = "https://httpbin.org/post";

    // Build POST body and headers using nlohmann/json
    json post_data = {
        {"name", "BotpySDK"},
        {"type", "plugin"}
    };
    std::string data_str = post_data.dump();

    json headers = {
        {"Content-Type", "application/json"},
        {"User-Agent", "BotpySDK/1.0"}
    };
    std::string headers_json_str = headers.dump();

    const char* resp = g_ctx.api.http_post_func(url, data_str.c_str(), headers_json_str.c_str());
    if (!resp) {
        send_msg(msg, "[SDK] HTTP POST returned NULL");
        return;
    }

    bool success = false;
    int status_code = 0;
    std::string body;
    std::string error;
    parse_http_response(resp, success, status_code, body, error);

    char reply[2048];
    snprintf(reply, sizeof(reply),
        "[SDK HTTP POST]\n"
        "  success: %s\n"
        "  status:  %d\n"
        "  error:   %s\n"
        "  body(len): %zu",
        success ? "true" : "false", status_code,
        error.empty() ? "(none)" : error.c_str(),
        body.size());
    send_msg(msg, reply);

    if (g_ctx.api.log_func) {
        char logbuf[1024];
        snprintf(logbuf, sizeof(logbuf), "HTTP POST result: success=%d status=%d body_len=%zu",
            success ? 1 : 0, status_code, body.size());
        g_ctx.api.log_func("info", logbuf);
    }
}

extern "C" __declspec(dllexport) int plugin_handle_message(const PluginMessage* msg) {
    if (!msg) return 0;


    const char* content = msg->content;
    if (msg->is_group) {
        while (*content == ' ' || *content == '\t') {
            content++;
        }
    }

    //if (g_ctx.api.send_msg_func && g_ctx.api.post_file_func) {
    //    const char* test_url = "https://foruda.gitee.com/avatar/1701004258171680525/5626221_meow_paz_1701004258.png";
    //    sendGroupImageMessage(msg, test_url, "", "Image sending failed");//发送图片封装

    //}

    // HTTP API command triggers
    if (_strnicmp(content, "/httpget", 8) == 0) {
        demo_http_get(msg);
    } else if (_strnicmp(content, "/httppost", 9) == 0) {
        demo_http_post(msg);
    }
    if (msg->is_group) {
        //群聊
        if (msg->is_groupat) {
            //艾特
        }
    }
    else {
        //私聊
    }

    return 0;
}


BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
