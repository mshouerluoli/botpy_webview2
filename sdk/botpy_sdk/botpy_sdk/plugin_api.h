#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*PluginLogFunc)(const char* level, const char* msg);
typedef int (*PluginSendMessageFunc)(const char* target_id, const char* content, int is_group);

typedef struct {
    const char* id;
    const char* content;
    const char* sender_id;
    const char* channel_id;
    int is_group;
    const char* openid;
    const char* group_openid;
} PluginMessage;

typedef struct {
    PluginLogFunc log_func;
    PluginSendMessageFunc send_msg_func;
    const char* appid;
    const char* data_path;
} PluginInitParams;

#define PLUGIN_API_VERSION 1

typedef int (*PluginInitFunc)(const PluginInitParams* params);
typedef int (*PluginHandleMessageFunc)(const PluginMessage* message);
typedef void (*PluginShutdownFunc)(void);
typedef const char* (*PluginGetNameFunc)(void);
typedef int (*PluginGetPriorityFunc)(void);
typedef const char* (*PluginGetAuthorFunc)(void);
typedef const char* (*PluginGetDescriptionFunc)(void);

#ifdef __cplusplus
}
#endif