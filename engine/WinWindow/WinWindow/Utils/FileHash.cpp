#include <WinWindow/pch.h>

#include <WinWindow/Utils/FileHash.h>

#include <bcrypt.h>

#pragma comment(lib, "bcrypt.lib")

namespace FDWWIN {

std::array<std::uint8_t, 32> FileHash::Sha256(const void* data, std::size_t size) {
    std::array<std::uint8_t, 32> digest{};
    BCRYPT_ALG_HANDLE algorithm = nullptr;
    BCRYPT_HASH_HANDLE hash = nullptr;
    DWORD objectSize = 0u;
    DWORD resultSize = 0u;
    if (!BCRYPT_SUCCESS(BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_SHA256_ALGORITHM, nullptr, 0u))) {
        return digest;
    }

    if (!BCRYPT_SUCCESS(BCryptGetProperty(
        algorithm,
        BCRYPT_OBJECT_LENGTH,
        reinterpret_cast<PUCHAR>(&objectSize),
        sizeof(objectSize),
        &resultSize,
        0u))) {
        BCryptCloseAlgorithmProvider(algorithm, 0u);
        return digest;
    }

    std::vector<std::uint8_t> object(objectSize);
    if (BCRYPT_SUCCESS(BCryptCreateHash(algorithm, &hash, object.data(), objectSize, nullptr, 0u, 0u)) &&
        BCRYPT_SUCCESS(BCryptHashData(
            hash,
            const_cast<PUCHAR>(static_cast<const UCHAR*>(data)),
            static_cast<ULONG>(size),
            0u))) {
        BCryptFinishHash(hash, digest.data(), static_cast<ULONG>(digest.size()), 0u);
    }

    if (hash) {
        BCryptDestroyHash(hash);
    }

    BCryptCloseAlgorithmProvider(algorithm, 0u);
    return digest;
}

std::string FileHash::ToHex(const std::array<std::uint8_t, 32>& digest) {
    static constexpr char digits[] = "0123456789abcdef";
    std::string result(digest.size() * 2u, '0');
    for (std::size_t index = 0u; index < digest.size(); ++index) {
        result[index * 2u] = digits[digest[index] >> 4u];
        result[index * 2u + 1u] = digits[digest[index] & 0x0fu];
    }

    return result;
}

}
