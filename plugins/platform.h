#ifndef _PLUGINS_PLATFORM_H_
#define _PLUGINS_PLATFORM_H_

#include <cstdio>

// 跨平台终端相关工具：在 POSIX 与 Windows 上提供一致的行为。
namespace plugins::platform {

// 判断给定流是否连接到终端。
bool isTerminal(std::FILE *stream);

// 返回终端宽度（列数）；无法获取时返回 0。
unsigned terminalWidth(std::FILE *stream);

// 在 Windows 控制台启用 UTF-8 输出；POSIX 上为空操作。
void enableUtf8Console();

} // namespace plugins::platform

#endif // !_PLUGINS_PLATFORM_H_
