#ifndef _CORE_SUM_SCAN_H_
#define _CORE_SUM_SCAN_H_
#include "plugins/file_list.h"
#include <string_view>

namespace sum {

void scan(std::string_view input, plugins::FileList &list);
}

#endif // !_CORE_SUM_SCAN_H_
