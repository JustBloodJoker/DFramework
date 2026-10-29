#pragma once

namespace FDWWIN {

class FileHash {
public:
    static std::array<std::uint8_t, 32> Sha256(const void* data, std::size_t size);
    static std::string ToHex(const std::array<std::uint8_t, 32>& digest);
};

}
