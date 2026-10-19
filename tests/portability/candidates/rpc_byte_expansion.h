#pragma once

#include <cstring>
#include <cstdint>

// Rejected production candidate: Clang changes the x87 C1 rounding status.
namespace PortableRpcQuantization
{
    inline void ExpandByte(int value, double inverseScale, float* output)
    {
        const float result = static_cast<float>(static_cast<double>(value) * inverseScale);
        // Publish the rounded bits without another numerical reload, which
        // would clear the original x87 rounding-direction status bit.
        std::uint32_t bits;
        std::memcpy(&bits, &result, sizeof(bits));
        const volatile std::uint32_t published = bits;
        const std::uint32_t snapshot = published;
        std::memcpy(output, &snapshot, sizeof(snapshot));
    }
}
