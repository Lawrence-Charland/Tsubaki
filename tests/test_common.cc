#include "common/common.h"
#include "common/help.h"
#include "plugins/file_list.h"
#include <gtest/gtest.h>
#include <iostream>
#include <sstream>
#include <string>

TEST(Common, FormatsFileSize) {
    EXPECT_EQ(common::formatFileSize(0), "0 B");
    EXPECT_EQ(common::formatFileSize(512), "512.00 B");
    EXPECT_EQ(common::formatFileSize(1024), "1.00 KiB");
    EXPECT_EQ(common::formatFileSize(1536), "1.50 KiB");
    EXPECT_EQ(common::formatFileSize(1024 * 1024), "1.00 MiB");
}

TEST(LoadFilesFromStream, ParsesTsubakiFormat) {
    std::istringstream is("aabbcc /tmp/a\n"
                          "# a comment\n"
                          "\n"
                          "  dd /tmp/b\n");
    plugins::FileList fl;
    common::loadFilesFromStream(fl, is, "test", "SUM");

    ASSERT_EQ(fl.size(), 2u);
    bool has_a = false;
    bool has_b = false;
    for (const auto &[key, val] : fl) {
        if (key == "/tmp/a") {
            has_a = true;
            EXPECT_EQ(val.hash, "aabbcc");
        } else if (key == "/tmp/b") {
            has_b = true;
            EXPECT_EQ(val.hash, "dd");
        }
    }
    EXPECT_TRUE(has_a);
    EXPECT_TRUE(has_b);
}

TEST(LoadFilesFromStream, RejectsInvalidHex) {
    std::istringstream is("zzz /tmp/a\n");
    plugins::FileList fl;
    common::loadFilesFromStream(fl, is, "test", "SUM");
    EXPECT_TRUE(fl.empty());
}

TEST(LoadFilesFromStream, RejectsLineWithoutPath) {
    std::istringstream is("aabbcc\n");
    plugins::FileList fl;
    common::loadFilesFromStream(fl, is, "test", "SUM");
    EXPECT_TRUE(fl.empty());
}

TEST(Help, PrintsKnownTopicAndRejectsUnknown) {
    std::ostringstream os;
    std::streambuf *old = std::cout.rdbuf(os.rdbuf());

    EXPECT_TRUE(common::printHelpTopic("sum", false));
    EXPECT_TRUE(common::printHelpTopic("--exclude", true));
    EXPECT_FALSE(common::printHelpTopic("does-not-exist", false));

    std::cout.rdbuf(old);

    const std::string out = os.str();
    EXPECT_NE(out.find("sum <algorithm>"), std::string::npos);
    EXPECT_NE(out.find("--exclude"), std::string::npos);
}
