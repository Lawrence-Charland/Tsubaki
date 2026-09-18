#ifndef _PLUGINS_TRIE_H_
#define _PLUGINS_TRIE_H_

#include <memory>
#include <string_view>
#include <unordered_map>

namespace plugins {
class Trie {
    struct TrieNode {
        std::unordered_map<char, std::unique_ptr<TrieNode>> children;
        bool is_end = false;
    };
    TrieNode m_root;

  public:
    void insert(std::string_view word);
    bool match(std::string_view) const;
};
} // namespace plugins

#endif // !_PLUGINS_TRIE_H_
