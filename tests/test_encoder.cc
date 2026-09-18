#include "encoder.h"
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <stdexcept>
#include <string>

TEST(Encoder, LookupKnownAndUnknown) {
    const auto &encoder = plugins::Encoder::GetInstance();
    EXPECT_NE(encoder.getMdByName("sha256"), nullptr);
    EXPECT_NE(encoder.getMdByName("md5"), nullptr);
    EXPECT_EQ(encoder.getMdByName("nope"), nullptr);
}

TEST(Encoder, EncodesAbcAsSha256) {
    namespace fs = std::filesystem;
    const auto path = fs::temp_directory_path() / "tsubaki_test_abc.txt";
    {
        std::ofstream out(path, std::ios::binary);
        out << "abc";
    }

    const auto *md = plugins::Encoder::GetInstance().getMdByName("sha256");
    ASSERT_NE(md, nullptr);
    const auto hash = plugins::Encoder::encodeFile(path.string(), 3, md);
    EXPECT_EQ(
        hash,
        "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");

    fs::remove(path);
}

TEST(Encoder, ThrowsOnNullAlgorithm) {
    EXPECT_THROW(plugins::Encoder::encodeFile("/tmp/x", 0, nullptr),
                 std::runtime_error);
}
