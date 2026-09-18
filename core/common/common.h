#ifndef _CORE_COMMON_COMMON_H_
#define _CORE_COMMON_COMMON_H_

#include "plugins/file_list.h"
#include <cstdint>
#include <istream>
#include <string>
#include <string_view>
#include <vector>
namespace common {

void setLogLevel();

// tsubaki 校验和列表中的一条记录
struct ChecksumEntry {
    std::string hash;
    std::string path;
    int mark = 0;
};

// 从流中读取 tsubaki 格式的校验和列表（每行 "<哈希> <路径>"）。
// 忽略空行与以 '#' 开头的行；非法行记入 errors 后继续解析。
void loadChecksumEntries(std::istream &is, std::string_view source,
                         std::vector<ChecksumEntry> &out,
                         std::vector<std::string> &errors);

// 将字节数格式化为可读字符串，例如 1536 -> "1.50 KiB"
std::string formatFileSize(uintmax_t bytes);

void loadFilesFromStream(plugins::FileList &fl, std::istream &is,
                         std::string_view name, std::string_view mode);
} // namespace common

#endif // !_CORE_COMMON_COMMON_H_
