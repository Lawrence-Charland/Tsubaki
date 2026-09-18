#ifndef _PLUGIN_PROGRESS_BAR_H_
#define _PLUGIN_PROGRESS_BAR_H_

#include <cstddef>
#include <ostream>
#include <string_view>

namespace plugins {

// 单行进度条，使用 '\r' 原地刷新。仅在主线程调用。
// enabled 为 false 时所有方法均为空操作，便于在非 TTY 下保持输出干净。
class ProgressBar {
  public:
    ProgressBar(std::size_t total, std::ostream &os, bool enabled);

    // done 为已完成数量，detail 为附加信息（例如已处理字节数）
    void update(std::size_t done, std::string_view detail = {});
    // 擦除当前行，用于在进度条上方打印日志
    void clear();
    // 结束进度条并擦除当前行
    void finish();

    // 当前终端宽度是否足以完整绘制进度条。
    // 探测失败（例如非 TTY）时返回 false。
    static bool terminalIsWideEnough();

    // 进度条是否启用。禁用时 update/clear/finish 均为空操作，
    // 调用方可据此跳过 detail 的格式化开销。
    bool enabled() const noexcept { return m_enabled; }

  private:
    std::size_t m_total;
    std::ostream &m_os;
    bool m_enabled;
    std::size_t m_step;
    std::size_t m_last_done = 0;
    std::size_t m_last_len = 0;

    void render(std::size_t done, std::string_view detail);
};

} // namespace plugins

#endif // !_PLUGIN_PROGRESS_BAR_H_
