#ifndef _PLUGIN_ENCODER_H_
#define _PLUGIN_ENCODER_H_

#include "plugin.h"
#include <cstdint>
#include <openssl/evp.h>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace plugins {

class Encoder : public Plugin<Encoder> {
    const std::unordered_map<std::string_view, const EVP_MD *> map = {

#define EVP_MAP_ITEM(name) {#name, EVP_##name()}
        EVP_MAP_ITEM(md4),        EVP_MAP_ITEM(md5),
        EVP_MAP_ITEM(sha1),       EVP_MAP_ITEM(sha224),
        EVP_MAP_ITEM(sha256),     EVP_MAP_ITEM(sha384),
        EVP_MAP_ITEM(sha3_224),   EVP_MAP_ITEM(sha3_256),
        EVP_MAP_ITEM(sha3_384),   EVP_MAP_ITEM(sha3_512),
        EVP_MAP_ITEM(sha512),     EVP_MAP_ITEM(sha512_224),
        EVP_MAP_ITEM(sha512_256), EVP_MAP_ITEM(shake128),
        EVP_MAP_ITEM(shake256),   EVP_MAP_ITEM(blake2b512),
        EVP_MAP_ITEM(blake2s256),
#undef EVP_MAP_ITEM
    };

  public:
    const EVP_MD *getMdByName(std::string_view name) const;
    static std::string encodeFile(std::string_view path, uintmax_t size,
                                  const EVP_MD *md);
};
} // namespace plugins

#endif
