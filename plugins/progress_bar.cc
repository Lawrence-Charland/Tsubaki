#include "progress_bar.h"
#include "platform.h"
#include <algorithm>
#include <format>
#include <string>

using namespace plugins;

namespace {
constexpr std::size_t kBarWidth = 30;
// 完整绘制进度条（含百分比、计数与详情）所需的最小终端列数。
// 终端窄于此值时换行会破坏 '\r' 原地刷新，因此不启用进度条。
constexpr std::size_t kMinTerminalWidth = 60;
}

ProgressBar::ProgressBar(std::size_t total, std::ostream &os, bool enabled)
    : m_total(total), m_os(os), m_enabled(enabled),
      m_step(std::max<std::size_t>(1, total / 100)) {}

void ProgressBar::update(std::size_t done, std::string_view detail) {
    if (!m_enabled)
        return;
    // 若当前行已被 clear() 擦除（m_last_len == 0），必须立即重绘，
    // 否则节流会导致进度条在两次刷新之间一直不可见。
    if (m_last_len != 0 && done != m_total && done - m_last_done < m_step)
        return;
    m_last_done = done;
    render(done, detail);
}

void ProgressBar::render(std::size_t done, std::string_view detail) {
    const bool finished = done >= m_total;
    double ratio = m_total == 0 ? 1.0 : static_cast<double>(done) / m_total;
    // 尚未全部完成时最多显示 99%，只有真正结束才到 100%。
    if (!finished)
        ratio = std::min(ratio, 0.99);
    const std::size_t filled = static_cast<std::size_t>(ratio * kBarWidth);

    std::string bar;
    bar.reserve(kBarWidth);
    for (std::size_t i = 0; i < kBarWidth; ++i) {
        bar += (i < filled ? '#' : '-');
    }

    std::string line =
        std::format("[{}] {:3.0f}% {}/{}", bar, ratio * 100.0, done, m_total);
    if (!detail.empty()) {
        line += std::format("  {}", detail);
    }
    if (line.size() < m_last_len) {
        line.append(m_last_len - line.size(), ' ');
    }
    m_last_len = line.size();

    m_os << '\r' << line << std::flush;
}

void ProgressBar::clear() {
    if (!m_enabled || m_last_len == 0)
        return;
    m_os << '\r' << std::string(m_last_len, ' ') << '\r' << std::flush;
    m_last_len = 0;
}

void ProgressBar::finish() {
    if (!m_enabled)
        return;
    // 程序结束时擦除进度条，避免残留在终端上。
    clear();
}

bool ProgressBar::terminalIsWideEnough() {
    if (!platform::isTerminal(stderr))
        return false;
    const unsigned width = platform::terminalWidth(stderr);
    return width >= kMinTerminalWidth;
}
