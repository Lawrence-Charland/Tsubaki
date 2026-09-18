#include "arg_parser.h"
#include <gtest/gtest.h>

TEST(ArgParser, ParsesCommandsAndOptions) {
    const char *raw[] = {"tsubaki",  "sum",     "sha256", "--max-size=1k",
                         "--force-scan", "/tmp"};
    auto &parser = plugins::ArgParser::GetInstance();
    parser.parse(6, const_cast<char **>(raw));

    const auto &commands = parser.getCommands();
    ASSERT_EQ(commands.size(), 3u);
    EXPECT_EQ(commands[0], "sum");
    EXPECT_EQ(commands[1], "sha256");
    EXPECT_EQ(commands[2], "/tmp");

    const auto *max_size = parser.getValue("--max-size");
    ASSERT_NE(max_size, nullptr);
    ASSERT_EQ(max_size->size(), 1u);
    EXPECT_EQ(max_size->front(), "1k");

    const auto *force_scan = parser.getValue("--force-scan");
    ASSERT_NE(force_scan, nullptr);
    EXPECT_TRUE(force_scan->empty());

    EXPECT_EQ(parser.getValue("--missing"), nullptr);
}
