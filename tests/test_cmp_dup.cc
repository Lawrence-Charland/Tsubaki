#include "cmp/cmp.h"
#include "common/common.h"
#include "dup/dup.h"
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <sstream>
#include <string>
#include <vector>

TEST(LoadChecksumEntries, ParsesAndReportsErrors) {
    std::istringstream in("aabbcc /tmp/a\n"
                          "# comment\n"
                          "\n"
                          "  dd /tmp/b  \n"
                          "nothex /tmp/c\n"
                          "nopath\n");
    std::vector<common::ChecksumEntry> entries;
    std::vector<std::string> errors;
    common::loadChecksumEntries(in, "test", entries, errors);

    ASSERT_EQ(entries.size(), 2u);
    EXPECT_EQ(entries[0].hash, "aabbcc");
    EXPECT_EQ(entries[0].path, "/tmp/a");
    EXPECT_EQ(entries[1].hash, "dd");
    EXPECT_EQ(entries[1].path, "/tmp/b");
    EXPECT_EQ(errors.size(), 2u);
}

TEST(Dup, GroupsIdenticalHashes) {
    std::istringstream in("aa a.txt\nbb b.txt\naa c.txt\n");
    std::ostringstream out;

    EXPECT_EQ(duplicate::run(in, out), 0);

    const std::string text = out.str();
    EXPECT_NE(text.find("a.txt"), std::string::npos);
    EXPECT_NE(text.find("c.txt"), std::string::npos);
    EXPECT_NE(text.find("rm '"), std::string::npos);
    EXPECT_EQ(text.find("b.txt"), std::string::npos);
}

TEST(Dup, IgnoresDuplicatePaths) {
    std::istringstream in("aa a.txt\naa a.txt\n");
    std::ostringstream out;

    EXPECT_EQ(duplicate::run(in, out), 0);
    EXPECT_EQ(out.str().find("Checksum:"), std::string::npos);
}

TEST(Cmp, ClassifiesDifferences) {
    namespace fs = std::filesystem;
    const fs::path dir = fs::temp_directory_path() / "tsubaki_cmp_test";
    fs::create_directories(dir);
    const fs::path a = dir / "A.txt";
    const fs::path b = dir / "B.txt";
    {
        std::ofstream(a) << "aa a.txt\nbb b.txt\ncc c.txt\n11 old.txt\n";
        std::ofstream(b)
            << "aa a.txt\ndd b.txt\ncc c.txt\n11 new.txt\nee d.txt\n";
    }

    std::ostringstream out;
    EXPECT_EQ(cmp::compare(a.string(), b.string(), out), 0);

    const std::string text = out.str();
    EXPECT_NE(text.find("[!] b.txt"), std::string::npos);
    EXPECT_NE(text.find("[D][1][A]old.txt"), std::string::npos);
    EXPECT_NE(text.find("[D][1][B]new.txt"), std::string::npos);
    EXPECT_NE(text.find("[U][B]d.txt"), std::string::npos);
    EXPECT_NE(text.find("[=] a.txt"), std::string::npos);
    EXPECT_NE(text.find("[=] c.txt"), std::string::npos);

    fs::remove_all(dir);
}
