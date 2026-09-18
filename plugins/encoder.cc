#include "encoder.h"
#include <cstddef>
#include <cstdint>
#include <format>
#include <openssl/evp.h>
#include <stdexcept>

#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#else
#include <fcntl.h>
#include <unistd.h>
#endif

using namespace plugins;

namespace {

size_t getBufferSize(uintmax_t size) {

    if (size < 0x400000) {
        if (size < 0x40000) {
            // <256k -> 8k
            return 0x2000;
        } else {
            // [256k,4m) -> 32k
            return 0x8000;
        }
    } else {
        if (size < 0x4000000) {
            // [4m,64m) -> 256k
            return 0x40000;
        } else {
            // >=64m -> 1m
            return 0x100000;
        }
    }
}

int openRead(const char *path) {
#ifdef _WIN32
    return _open(path, _O_RDONLY | _O_BINARY);
#else
    return ::open(path, O_RDONLY);
#endif
}

void closeRead(int fd) {
#ifdef _WIN32
    _close(fd);
#else
    ::close(fd);
#endif
}

std::string toHex(const std::vector<unsigned char> &digest) {
    static const char *map = "0123456789abcdef";
    std::string res;
    res.reserve(digest.size() << 1);
    for (unsigned char c : digest) {
        res += map[c >> 4];
        res += map[c & 15];
    }
    return res;
}

}; // namespace

const EVP_MD *Encoder::getMdByName(std::string_view name) const {
    if (const auto it = map.find(name); it != map.end()) {
        return it->second;
    }
    return nullptr;
}
std::string Encoder::encodeFile(std::string_view path, uintmax_t file_size,
                                const EVP_MD *md) {
    if (!md)
        throw std::runtime_error("Sum: null digest algorithm.");

    // 直接使用文件描述符读取，避免 stdio 的锁与额外拷贝。
    const int fd = openRead(path.data());
    if (fd < 0)
        throw std::runtime_error(std::format("Sum: open file {} error.", path));

    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    EVP_DigestInit_ex(ctx, md, nullptr);

    // 获取文件大小并计算缓冲区大小
    const size_t buffer_size = getBufferSize(file_size);

    // 创建缓冲区并读取hash，线程复用；只增不减，避免反复清零。
    thread_local static std::vector<unsigned char> buf;
    if (buf.size() < buffer_size)
        buf.resize(buffer_size);

    for (;;) {
#ifdef _WIN32
        const int read_size =
            _read(fd, buf.data(), static_cast<unsigned int>(buffer_size));
#else
        const ssize_t read_size = ::read(fd, buf.data(), buffer_size);
#endif
        if (read_size < 0) {
            closeRead(fd);
            EVP_MD_CTX_free(ctx);
            throw std::runtime_error(
                std::format("Sum: read file {} error.", path));
        }
        if (read_size == 0)
            break;
        EVP_DigestUpdate(ctx, buf.data(), static_cast<size_t>(read_size));
    }
    closeRead(fd);

    std::vector<unsigned char> digest(EVP_MAX_MD_SIZE);
    unsigned int len = 0;
    EVP_DigestFinal_ex(ctx, digest.data(), &len);

    EVP_MD_CTX_free(ctx);
    digest.resize(len);
    return toHex(digest);
}
