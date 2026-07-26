// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "framework.h"


#include "plugin_api.h"
#include <cstring>
#include <cstdio>
#include <string>

static PluginLogFunc g_log = nullptr;
static PluginSendMessageFunc g_send_msg = nullptr;
static std::string data_dir;

extern "C" __declspec(dllexport) int plugin_init(const PluginInitParams* params) {
    if (!params) return -1;
    g_log = params->log_func;
    g_send_msg = params->send_msg_func;
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



extern "C" __declspec(dllexport) int plugin_handle_message(const PluginMessage* msg) {
    if (!msg) return 0;


    const char* content = msg->content;
    if (msg->is_group) {
        while (*content == ' ' || *content == '\t') {
            content++;
        }
    }

    if (g_send_msg) {
        if (msg->is_group && msg->group_openid) {
            g_send_msg(msg->group_openid, "Hello from plugin!", 1);
        }
        else if (!msg->is_group && msg->openid) {
            g_send_msg(msg->openid, "Hello from plugin!", 0);
        }
        return 1;//拦截优先级低的插件信息
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

