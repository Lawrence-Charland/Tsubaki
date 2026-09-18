#ifndef _CORE_DUP_DUP_H_
#define _CORE_DUP_DUP_H_

#include <iosfwd>

namespace duplicate {

// 从 in 读取校验和列表，把哈希相同的文件归为一组，输出到 out，
// 并给出建议的删除命令。返回进程退出码。
int run(std::istream &in, std::ostream &out);

// 从 stdin 读取并调用 run()。
int invoke();

} // namespace duplicate

#endif // !_CORE_DUP_DUP_H_
