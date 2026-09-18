#ifndef _PLUGINS_FILE_LIST_H_
#define _PLUGINS_FILE_LIST_H_

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>

namespace plugins {

class FileList {
  public:
    // 尚未取得文件大小时的占位值
    static constexpr uintmax_t unknown_size = UINTMAX_MAX;

    struct FileInfo {
        uintmax_t size = unknown_size;
        std::string hash;
    };

    using filesum_t = std::unordered_map<std::string, FileInfo>;
    using iterator = filesum_t::iterator;
    using const_iterator = filesum_t::const_iterator;

  private:
    filesum_t m_filesums;

  public:
    FileList() = default;
    ~FileList() = default;

    // 添加文件，返回原先是否存在
    bool add(std::string);
    // 添加文件并记录已知大小，返回原先是否存在
    bool add(std::string, uintmax_t size);
    // 设置hash
    void set(std::string_view, std::string_view);

    void initSizes();
    uintmax_t getTotalSize(bool);

    iterator begin() noexcept { return m_filesums.begin(); }
    iterator end() noexcept { return m_filesums.end(); }
    const_iterator begin() const noexcept { return m_filesums.begin(); }
    const_iterator end() const noexcept { return m_filesums.end(); }

    bool empty() const noexcept { return m_filesums.empty(); }
    std::size_t size() const noexcept { return m_filesums.size(); }

    template <typename Pred> void eraseIf(Pred &&pred) {
        std::erase_if(m_filesums, std::forward<Pred>(pred));
    }
};
} // namespace plugins

#endif // !_PLUGINS_FILE_LIST_H_
