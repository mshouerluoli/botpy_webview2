#include "framework.h"


#include "plugin_api.h"
#include <cstring>
#include <cstdio>
#include <string>

static PluginLogFunc g_log = nullptr;
static PluginSendMessageFunc g_send_msg = nullptr;
static PluginPostFileFunc g_post_file = nullptr;
static std::string data_dir;

extern "C" __declspec(dllexport) int plugin_init(const PluginInitParams* params) {
    if (!params) return -1;
    g_log = params->log_func;
    g_send_msg = params->send_msg_func;
    g_post_file = params->post_file_func;
    data_dir = params->data_path;
    if (g_log) g_log("info", "MyPlugin loaded");
    if (g_log) {
        std::string full_msg = "data_dir: " + data_dir;
        g_log("info", full_msg.c_str());
    }
    return 0;
}
extern "C" __declspec(dllexport) void plugin_shutdown() {
    if (g_log) g_log("info", "MyPlugin unloaded");
}

extern "C" __declspec(dllexport) const char* plugin_get_name() {
    return "MyPlugin";
}
extern "C" __declspec(dllexport) const char* plugin_get_author(void) {
    return "Miaopasi";
}

extern "C" __declspec(dllexport) const char* plugin_get_description(void) {
    return "这是一个示例SDK";
}
extern "C" __declspec(dllexport) int plugin_get_priority() {
    return 100;
}

void send_msg(const PluginMessage* msg, const char* Text ,int msg_type = 0, const char* media = nullptr) {
    if (g_send_msg) {
        if (msg->is_group && msg->group_openid) {
            g_send_msg(msg->group_openid, Text, 1, msg->id, msg_type, media);
        }
        else if (!msg->is_group && msg->openid) {
            g_send_msg(msg->openid, Text, 0, msg->id, msg_type, media);
        }

    }

}

void sendGroupImageMessage(const PluginMessage* msg, const char* imageUrl, const char* successText, const char* failText) {
    if (!msg || !g_post_file) return;

    const char* media = nullptr;
    if (msg->is_group) {
        media = g_post_file(msg->group_openid, imageUrl, 1, 1, 0);
    }
    else {
        media = g_post_file(msg->openid, imageUrl, 1, 0, 0);
    }
    

    if (media) {
        send_msg(msg, successText, 7, media);
    }
    else {
        send_msg(msg, failText, 0, media);
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

    if (g_send_msg && g_post_file) {
        const char* test_url = "https://foruda.gitee.com/avatar/1701004258171680525/5626221_meow_paz_1701004258.png";

        sendGroupImageMessage(msg, test_url, "", "Image sending failed");

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

