#ifndef _CORE_CMP_CMP_H_
#define _CORE_CMP_CMP_H_

#include <iosfwd>
#include <string>

namespace cmp {

// 比较两个 tsubaki 校验和列表文件（A、B），按路径与哈希分类：
// 修改、移动/复制、删除/新增、匹配，结果写入 out。返回进程退出码。
int compare(const std::string &file_a, const std::string &file_b,
            std::ostream &out);

// 从命令行参数读取两个文件并调用 compare()。
int invoke();

} // namespace cmp

#endif // !_CORE_CMP_CMP_H_
