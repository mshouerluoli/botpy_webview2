#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*PluginLogFunc)(const char* level, const char* msg);
typedef int (*PluginSendMessageFunc)(const char* target_id, const char* content, int is_group, const char* msg_id, int msg_type, const char* media);
typedef const char* (*PluginPostFileFunc)(const char* target_id, const char* url, int file_type, int is_group, int srv_send_msg);
typedef const char* (*PluginHttpGetFunc)(const char* url, const char* headers_json);
typedef const char* (*PluginHttpPostFunc)(const char* url, const char* data, const char* headers_json);

typedef struct {
    const char* id;
    const char* content;
    const char* sender_id;
    const char* channel_id;
    int is_group;
    int is_groupat;
    const char* openid;
    const char* group_openid;
} PluginMessage;

typedef struct {
    PluginLogFunc log_func;
    PluginSendMessageFunc send_msg_func;
    PluginPostFileFunc post_file_func;
    PluginHttpGetFunc http_get_func;
    PluginHttpPostFunc http_post_func;
    const char* appid;
    const char* data_path;
} PluginInitParams;

#define PLUGIN_API_VERSION 1

typedef int (*PluginInitFunc)(const PluginInitParams* params);
typedef int (*PluginHandleMessageFunc)(const PluginMessage* message);
typedef void (*PluginShutdownFunc)(void);
typedef const char* (*PluginGetNameFunc)(void);
typedef int (*PluginGetPriorityFunc)(void);

#ifdef __cplusplus
}
#endif