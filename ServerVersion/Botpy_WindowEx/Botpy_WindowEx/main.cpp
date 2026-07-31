#include <windows.h>
#include <thread>
#include <chrono>
#include "MainWindow.h"
#include "Miao.h"
#include "json.hpp"

using json = nlohmann::json;

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR lpCmdLine, int nCmdShow) {
    SetProcessDPIAware();

    // 获取可执行文件目录
    wchar_t exe_path[MAX_PATH];
    GetModuleFileNameW(NULL, exe_path, MAX_PATH);
    std::wstring exe_dir_w = exe_path;
    size_t pos = exe_dir_w.find_last_of(L"\\/");
    if (pos != std::wstring::npos) {
        exe_dir_w = exe_dir_w.substr(0, pos);
    }
    std::string exe_dir(exe_dir_w.begin(), exe_dir_w.end());
    std::string config_path = exe_dir + "\\config.yaml";

    // 加载配置
    Config config;
    if (!config.load_from_file(config_path)) {
        MessageBoxA(NULL, ("Config file not found: " + config_path + "\nPlease create config.yaml with appid and secret").c_str(), "Miao Bot Warning", MB_ICONWARNING);
        // 继续运行，即使没有配置文件
    } else if (config.appid.empty() || config.secret.empty()) {
        MessageBoxA(NULL, "appid or secret is empty in config.yaml", "Miao Bot Warning", MB_ICONWARNING);
    }

    // 创建主窗口
    MainWindow window;

    if (!window.Create(hInst, nCmdShow)) {
        MessageBoxW(nullptr, L"Failed to create window!", L"Error", MB_ICONERROR);
        return 1;
    }

    // 初始化机器人客户端
    MyClient client;
    static MainWindow* g_window = &window;  // 用于回调

    // 设置主窗口指针（用于插件日志回调）
    SetMainWindowPointer(&window);

    // 设置机器人回调
    client.on_log = [&window](const std::string& level, const std::string& msg) {
        window.PostLog(level, msg);
    };
    client.on_status = [&window](const std::string& status, const std::string& text) {
        window.PostStatus(status, text);
    };
    client.on_info = [&window](const std::string& id, const std::string& value) {
        window.PostInfo(id, value);
    };
    client.on_message = [&window](bool is_group, const std::string& username, const std::string& content) {
        window.PostMessageEvent(is_group, username,content);
    };
    client.on_restart = [&client, &config]() {
        std::this_thread::sleep_for(std::chrono::seconds(3));
        try {
            client.run(config.appid, config.secret, config.worker_count);
        } catch (...) {
        }
    };

    // 设置命令处理器
    window.SetCommandHandler([&client, &window, &config](const std::string& cmd) {
        try {
            if (cmd == "stop") {
                client.stop();
            } else if (cmd == "get_plugins") {
                auto plugins = client.get_plugins();
                json j;
                j["type"] = "plugins";
                json::array_t arr;
                for (auto& p : plugins) {
                    json item;
                    item["name"] = p.name;
                    item["display_name"] = p.display_name;
                    item["author"] = p.author;
                    item["description"] = p.description;
                    item["enabled"] = p.enabled;
                    item["priority"] = p.priority;
                    arr.push_back(item);
                }
                j["plugins"] = arr;
                window.PostPlugins(j.dump());
            } else if (cmd.rfind("toggle_plugin:", 0) == 0) {
                std::string plugin_name = cmd.substr(14);
                auto plugins = client.get_plugins();
                bool current_enabled = true;
                for (auto& p : plugins) {
                    if (p.name == plugin_name) {
                        current_enabled = p.enabled;
                        break;
                    }
                }
                client.toggle_plugin(plugin_name, !current_enabled);
                auto plugins_after = client.get_plugins();
                json j;
                j["type"] = "plugins";
                json::array_t arr;
                for (auto& p : plugins_after) {
                    json item;
                    item["name"] = p.name;
                    item["display_name"] = p.display_name;
                    item["author"] = p.author;
                    item["description"] = p.description;
                    item["enabled"] = p.enabled;
                    item["priority"] = p.priority;
                    arr.push_back(item);
                }
                j["plugins"] = arr;
                window.PostPlugins(j.dump());
            } else if (cmd.rfind("unload_plugin:", 0) == 0) {
                std::string plugin_name = cmd.substr(14);
                client.unload_plugin(plugin_name);
                auto plugins = client.get_plugins();
                json j;
                j["type"] = "plugins";
                json::array_t arr;
                for (auto& p : plugins) {
                    json item;
                    item["name"] = p.name;
                    item["display_name"] = p.display_name;
                    item["author"] = p.author;
                    item["description"] = p.description;
                    item["enabled"] = p.enabled;
                    item["priority"] = p.priority;
                    arr.push_back(item);
                }
                j["plugins"] = arr;
                window.PostPlugins(j.dump());
            } else if (cmd == "reload_plugins") {
                client.reload_plugins(config.appid);
                auto plugins = client.get_plugins();
                json j;
                j["type"] = "plugins";
                json::array_t arr;
                for (auto& p : plugins) {
                    json item;
                    item["name"] = p.name;
                    item["display_name"] = p.display_name;
                    item["author"] = p.author;
                    item["description"] = p.description;
                    item["enabled"] = p.enabled;
                    item["priority"] = p.priority;
                    arr.push_back(item);
                }
                j["plugins"] = arr;
                window.PostPlugins(j.dump());
            }
        } catch (const std::exception& e) {
            window.PostLog("error", "Command handler exception: " + std::string(e.what()));
        } catch (...) {
            window.PostLog("error", "Command handler unknown exception");
        }
    });

    // 设置插件管理回调
    window.SetPluginGetHandler([&client]() -> std::vector<PluginInstance> {
        return client.get_plugins();
    });
    window.SetPluginToggleHandler([&client](const std::string& name, bool enable) -> bool {
        return client.toggle_plugin(name, enable);
    });
    window.SetPluginUnloadHandler([&client](const std::string& name) -> bool {
        return client.unload_plugin(name);
    });
    window.SetPluginReloadHandler([&client, &config](const std::string& appid) -> bool {
        return client.reload_plugins(config.appid);
    });

    // 设置就绪处理器（启动机器人）
    window.SetReadyHandler([&client, &config, &window]() {
        // 延迟启动机器人
        std::thread bot_thread([&client, &config, &window]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            
            if (config.appid.empty() || config.secret.empty()) {
                window.PostLog("warning", "未配置config.yaml，请添加appid和secret");
                window.PostLog("info", "创建config.yaml文件：");
                window.PostLog("info", "  appid: \"your_app_id\"");
                window.PostLog("info", "  secret: \"your_secret\"");
                return;
            }
            
            try {
                client.run(config.appid, config.secret, config.worker_count);
            } catch (const std::exception& e) {
                window.PostLog("error", std::string("Bot start exception: ") + e.what());
            } catch (...) {
                window.PostLog("error", "Bot start unknown exception");
            }
        });
        bot_thread.detach();
    });

    // 使用 PostMessage 延迟初始化数据
    HWND hwnd = window.GetHwnd();
    if (hwnd) {
        PostMessageW(hwnd, WM_USER + 2, 0, 0);
    }

    // 运行消息循环（会调用 SetReadyHandler）
    window.RunMessageLoop();

    // 停止机器人
    client.stop();
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    return 0;
}