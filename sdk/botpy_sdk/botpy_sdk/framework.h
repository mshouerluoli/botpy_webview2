#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string>

namespace Plugin {
	inline std::string name = "MyPlugin";//插件名称
	inline std::string author = "Miaopasi";//作者名
	inline std::string description = "这是一个示例SDK";//说明
	inline int priority = 100;//优先级 1最大 plugin_handle_message 返回1即可拦截比这个值小的插件,要拦截100优先级的插件，那么你就得写99
}