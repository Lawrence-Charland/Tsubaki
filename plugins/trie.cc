#include "trie.h"
#include <string_view>

using namespace plugins;

void Trie::insert(std::string_view word) {
    TrieNode *cur = &m_root;
    for (char c : word) {
        auto &next = cur->children[c];
        if (!next)
            next = std::make_unique<TrieNode>();
        cur = next.get();
    }
    cur->is_end = true;
}
bool Trie::match(std::string_view pattern) const {
    const TrieNode *cur = &m_root;
    for (char c : pattern) {
        auto it = cur->children.find(c);
        if (it == cur->children.end()) {
            return false;
        }
        cur = it->second.get();
        if (!cur) [[unlikely]] {
            return false;
        }
        if (cur->is_end) {
            return true;
        }
    }
    return false;
}
