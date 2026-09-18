#include "trie.h"
#include <gtest/gtest.h>

TEST(Trie, MatchesExactAndPrefix) {
    plugins::Trie trie;
    trie.insert("/home/user/.cache");

    EXPECT_TRUE(trie.match("/home/user/.cache"));
    EXPECT_TRUE(trie.match("/home/user/.cache/file.txt"));
    EXPECT_FALSE(trie.match("/home/user/.config"));
    EXPECT_FALSE(trie.match("/home"));
    EXPECT_FALSE(trie.match("/"));
}

TEST(Trie, MatchesMultipleBranches) {
    plugins::Trie trie;
    trie.insert("/a/b");
    trie.insert("/a/c");

    EXPECT_TRUE(trie.match("/a/b/x"));
    EXPECT_TRUE(trie.match("/a/c/y"));
    EXPECT_FALSE(trie.match("/a/d"));
}

TEST(Trie, EmptyTrieNeverMatches) {
    plugins::Trie trie;
    EXPECT_FALSE(trie.match("anything"));
}
