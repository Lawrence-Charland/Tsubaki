#include "file_list.h"
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <string>

TEST(FileList, AddRejectsDuplicates) {
    plugins::FileList fl;
    EXPECT_TRUE(fl.add("/a"));
    EXPECT_FALSE(fl.add("/a"));
    EXPECT_EQ(fl.size(), 1u);
}

TEST(FileList, AddWithKnownSize) {
    plugins::FileList fl;
    fl.add("/a", 123);

    ASSERT_EQ(fl.size(), 1u);
    EXPECT_EQ(fl.begin()->second.size, 123u);
    EXPECT_EQ(fl.getTotalSize(false), 123u);
}

TEST(FileList, EraseIfRemovesMatchingEntries) {
    plugins::FileList fl;
    fl.add("/a", 10);
    fl.add("/b", 20);

    fl.eraseIf([](const plugins::FileList::filesum_t::value_type &item) {
        return item.second.size > 15;
    });

    ASSERT_EQ(fl.size(), 1u);
    EXPECT_EQ(fl.begin()->first, "/a");
}

TEST(FileList, InitSizesFillsUnknownEntries) {
    namespace fs = std::filesystem;
    const auto path = fs::temp_directory_path() / "tsubaki_test_init_sizes.txt";
    {
        std::ofstream out(path, std::ios::binary);
        out << "hello";
    }

    plugins::FileList fl;
    fl.add(path.string());
    fl.initSizes();

    ASSERT_EQ(fl.size(), 1u);
    EXPECT_EQ(fl.begin()->second.size, 5u);
    fs::remove(path);
}

TEST(FileList, InitSizesDropsMissingFiles) {
    plugins::FileList fl;
    fl.add("/definitely/not/here/tsubaki");
    fl.initSizes();
    EXPECT_TRUE(fl.empty());
}
