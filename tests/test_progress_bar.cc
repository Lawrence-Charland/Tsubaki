#include "progress_bar.h"
#include <gtest/gtest.h>
#include <sstream>
#include <string>

TEST(ProgressBar, RendersProgressAndDetail) {
    std::ostringstream os;
    plugins::ProgressBar bar(4, os, true);

    bar.update(2, "1.00 KiB / 2.00 KiB");
    bar.update(4, "2.00 KiB / 2.00 KiB");
    bar.finish();

    const std::string out = os.str();
    EXPECT_NE(out.find("50%"), std::string::npos);
    EXPECT_NE(out.find("100%"), std::string::npos);
    EXPECT_NE(out.find("2/4"), std::string::npos);
    EXPECT_NE(out.find("1.00 KiB / 2.00 KiB"), std::string::npos);
}

TEST(ProgressBar, CapsAtNinetyNineUntilFinished) {
    std::ostringstream os;
    plugins::ProgressBar bar(1000, os, true);

    bar.update(999);
    EXPECT_NE(os.str().find("99%"), std::string::npos);
    EXPECT_EQ(os.str().find("100%"), std::string::npos);

    bar.update(1000);
    EXPECT_NE(os.str().find("100%"), std::string::npos);
}

TEST(ProgressBar, DisabledWritesNothing) {
    std::ostringstream os;
    plugins::ProgressBar bar(4, os, false);

    bar.update(2, "detail");
    bar.update(4, "detail");
    bar.clear();
    bar.finish();

    EXPECT_TRUE(os.str().empty());
}

TEST(ProgressBar, ClearErasesCurrentLine) {
    std::ostringstream os;
    plugins::ProgressBar bar(10, os, true);

    bar.update(5);
    bar.clear();

    const std::string out = os.str();
    EXPECT_NE(out.find('\r'), std::string::npos);
    EXPECT_NE(out.find("50%"), std::string::npos);
}
