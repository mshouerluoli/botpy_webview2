#pragma once

#include <string>
#include <memory>
#include <atomic>
#include <mutex>
#include <thread>
#include <functional>
#include <chrono>
#include <vector>
#include <windows.h>

#include "http/websocket_client.hpp"
#include "plugin_api.h"
#include "message_queue.h"

struct Config {
    std::string appid;
    std::string secret;
    int worker_count;
    bool load_from_file(const std::string& path);
};

struct Message {
    std::string id;
    std::string content;
    std::string sender_id;
    std::string channel_id;
    bool is_group;
    std::string openid;
    std::string group_openid;
};

struct C2CMessage : Message {};
struct GroupMessage : Message {};

struct PluginInstance {
    HMODULE handle;
    std::string name;
    std::string display_name;
    std::string author;
    std::string description;
    bool enabled;
    int priority;
    PluginHandleMessageFunc handle_message;
    PluginInitFunc init;
    PluginShutdownFunc shutdown;
    PluginGetNameFunc get_name;
    PluginGetPriorityFunc get_priority;
    const char* (*get_author)(void);
    const char* (*get_description)(void);
};

class PluginManager {
public:
    PluginManager();
    ~PluginManager();
    
    void load_plugins(const std::string& appid);
    void unload_plugins();
    void handle_message(const Message& message, bool is_group);
    
    std::vector<PluginInstance> get_plugins();
    bool toggle_plugin(const std::string& name, bool enable);
    bool unload_plugin(const std::string& name);
    bool reload_plugins(const std::string& appid);
    
private:
    std::vector<PluginInstance> m_plugins;
    std::mutex m_mutex;
};

class MyClient {
public:
    MyClient();
    ~MyClient();
    
    void run(const std::string& appid, const std::string& secret, int worker_count = 4);
    void stop();
    
    bool send_c2c_message(const std::string& openid, const std::string& content, const std::string& msg_id = "", int msg_type = 0, const std::string& media = "") {
        return _send_c2c_message(openid, content, msg_id, msg_type, media);
    }
    bool send_group_message(const std::string& group_openid, const std::string& content, const std::string& msg_id = "", int msg_type = 0, const std::string& media = "") {
        return _send_group_message(group_openid, content, msg_id, msg_type, media);
    }
    std::string post_c2c_file(const std::string& openid, const std::string& url, int file_type, bool srv_send_msg = false) {
        return _post_c2c_file(openid, url, file_type, srv_send_msg);
    }
    std::string post_group_file(const std::string& group_openid, const std::string& url, int file_type, bool srv_send_msg = false) {
        return _post_group_file(group_openid, url, file_type, srv_send_msg);
    }
    
    std::vector<PluginInstance> get_plugins() { return m_plugin_manager.get_plugins(); }
    bool toggle_plugin(const std::string& name, bool enable) { return m_plugin_manager.toggle_plugin(name, enable); }
    bool unload_plugin(const std::string& name) { return m_plugin_manager.unload_plugin(name); }
    bool reload_plugins(const std::string& appid) { return m_plugin_manager.reload_plugins(appid); }
    
    std::function<void(const std::string& level, const std::string& msg)> on_log;
    std::function<void(const std::string& status, const std::string& text)> on_status;
    std::function<void(const std::string& id, const std::string& value)> on_info;
    std::function<void(bool is_group, const std::string& content)> on_message;
    std::function<void()> on_restart;

protected:
    virtual void on_ready();
    virtual void on_c2c_message_create(const C2CMessage& message);
    virtual void on_group_at_message_create(const GroupMessage& message);
    
private:
    bool _authenticate();
    bool _get_gateway_url();
    bool _connect_websocket();
    bool _reconnect();
    void _websocket_listener();
    void _send_heartbeat();
    void _send_identify();
    void _handle_event(const std::string& event_json);
    void _handle_common_commands(const Message& message, bool message_isgroup);
    void _worker_thread();
    void _start_worker_pool();
    void _stop_worker_pool();
    void _process_message_task(const MessageTask& task);

    bool _send_c2c_message(const std::string& openid, const std::string& content, const std::string& msg_id = "", int msg_type = 0, const std::string& media = "");
    bool _send_group_message(const std::string& group_openid, const std::string& content, const std::string& msg_id = "", int msg_type = 0, const std::string& media = "");
    std::string _post_c2c_file(const std::string& openid, const std::string& url, int file_type, bool srv_send_msg = false);
    std::string _post_group_file(const std::string& group_openid, const std::string& url, int file_type, bool srv_send_msg = false);
    std::string _build_msg_json(const std::string& content, const std::string& msg_id = "", int msg_type = 0, const std::string& media = "");
    
    void log(const std::string& level, const std::string& msg);

    std::string m_appid;
    std::string m_secret;
    std::string m_token;
    std::string m_gateway_url;
    std::atomic<bool> m_running;
    std::atomic<bool> m_websocket_connected;
    
    std::mutex m_websocket_mutex;
    WebSocketClient* m_websocket;
    std::thread m_heartbeat_thread;
    
    int m_heartbeat_interval;
    std::string m_session_id;
    int m_sequence;
    std::atomic<int> m_message_count;
    int m_heartbeat_count;
    std::string m_nickname;
    std::atomic<uint64_t> m_last_message_ts;
    
    MessageQueue m_message_queue;
    std::vector<std::thread> m_worker_threads;
    int m_worker_count;
    
    PluginManager m_plugin_manager;
};

class Intents {
public:
    bool guilds;
    bool guild_members;
    bool guild_messages;
    bool guild_message_reactions;
    bool direct_message;
    bool open_forum_event;
    bool audio_or_live_channel_member;
    bool public_messages;
    bool interaction;
    bool message_audit;
    bool forums;
    bool audio_action;
    bool public_guild_messages;

    Intents() :
        guilds(false), guild_members(false),
        guild_messages(false), guild_message_reactions(false),
        direct_message(false),
        open_forum_event(false), audio_or_live_channel_member(false),
        public_messages(false),
        interaction(false), message_audit(false),
        forums(false), audio_action(false),
        public_guild_messages(false) {}

    Intents(bool pm, bool dm) :
        guilds(false), guild_members(false),
        guild_messages(false), guild_message_reactions(false),
        direct_message(dm),
        open_forum_event(false), audio_or_live_channel_member(false),
        public_messages(pm),
        interaction(false), message_audit(false),
        forums(false), audio_action(false),
        public_guild_messages(false) {}

    uint32_t to_bits() const;
};