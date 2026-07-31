#pragma once

#define WIN32_LEAN_AND_MEAN             // 从 Windows 头文件中排除极少使用的内容
// Windows 头文件
#include <windows.h>
#include <string>

namespace Plugin {
	inline std::string name = "MyPlugin";
	inline std::string author = "Miaopasi";
	inline std::string description = "这是一个示例SDK";
	inline int priority = 100;
}