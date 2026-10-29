#include <pch.h>
#include <ShadowData/NeuralShadowData.h>

namespace FDW_SD::Output {

float VisibilityEncodingTools::Clamp(float visibility) {
    return std::isfinite(visibility) ? std::clamp(visibility, 0.0f, 1.0f) : 1.0f;
}

std::uint8_t VisibilityEncodingTools::EncodeR8Unorm(float visibility) {
    return static_cast<std::uint8_t>(std::floor(Clamp(visibility) * 255.0f + 0.5f));
}

float VisibilityEncodingTools::DecodeR8Unorm(std::uint8_t encoded) {
    return static_cast<float>(encoded) / 255.0f;
}

std::uint16_t VisibilityEncodingTools::EncodeR16Float(float visibility) {
    const auto bits = std::bit_cast<std::uint32_t>(Clamp(visibility));
    const auto exponent = (bits >> 23u) & 0xffu;
    
    std::uint32_t mantissa = bits & 0x7fffffu;
    if (exponent < 103u) return 0u;
    
    if (exponent < 113u) {
        
        mantissa |= 0x800000u;
        
        const auto shift = 126u - exponent;
        auto halfMantissa = mantissa >> shift;
        
        const auto remainderMask = (1u << shift) - 1u;
        const auto remainder = mantissa & remainderMask;
        const auto halfway = 1u << (shift - 1u);
        
        if (remainder > halfway || (remainder == halfway && (halfMantissa & 1u) != 0u))
            halfMantissa++;
        
        return static_cast<std::uint16_t>(halfMantissa);
    }

    std::uint32_t halfExponent = exponent - 112u;
    std::uint32_t halfMantissa = mantissa >> 13u;
    
    const auto remainder = mantissa & 0x1fffu;
    
    if (remainder > 0x1000u || (remainder == 0x1000u && (halfMantissa & 1u) != 0u)) {
        halfMantissa++;
        if (halfMantissa == 0x400u) {
            halfMantissa = 0u;
            halfExponent++;
        }
    }
    
    return static_cast<std::uint16_t>((halfExponent << 10u) | halfMantissa);
}

float VisibilityEncodingTools::DecodeR16Float(std::uint16_t encoded) {
    const auto exponent = (encoded >> 10u) & 0x1fu;
    const auto mantissa = encoded & 0x3ffu;
    
    if (exponent == 0u) return std::ldexp(static_cast<float>(mantissa), -24);
    
    return std::ldexp(1.0f + static_cast<float>(mantissa) / 1024.0f, static_cast<int>(exponent) - 15);
}

} // namespace FDW_SD::Output
