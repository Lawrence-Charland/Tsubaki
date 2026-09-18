#ifndef _CORE_COMMON_HELP_H_
#define _CORE_COMMON_HELP_H_

#include <string_view>

namespace common {

// 输出总帮助信息；chinese 为 true 时输出中文，否则输出英文
void printHelp(bool chinese);

// 输出指定主题（命令、输入、选项或算法）的详细帮助。
// 返回 false 表示该主题不存在。
bool printHelpTopic(std::string_view topic, bool chinese);

} // namespace common

#endif // !_CORE_COMMON_HELP_H_
