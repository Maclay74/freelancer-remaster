#include "candidates/param_strict_less.h"
#include <cstdint>
#include <cstdio>
#include <cstring>

__declspec(noinline) bool Reference(const float* left, const float* right)
{
    unsigned char less;
    __asm {
        mov ecx, left
        fld dword ptr [ecx]
        mov ecx, right
        fcomp dword ptr [ecx]
        fnstsw ax
        test ah, 05h
        setnp less
    }
    return less != 0;
}

__declspec(noinline) bool Candidate(const float* left, const float* right)
{
    return ThornComparisonCandidate::StrictLess(left, right);
}

int main()
{
    const std::uint32_t patterns[] = {0, 0x80000000u, 0x3f800000u, 0xbf800000u,
        0x7f800000u, 0xff800000u, 0x7fc12345u, 0xffc54321u, 0x7f812345u,
        0xff854321u, 1, 0x80000001u, 0x007fffffu, 0x00800000u, 0x7f7fffffu};
    unsigned short original;
    __asm { fnstcw original }
    unsigned cases = 0, results = 0, flags = 0;
    std::uint32_t random = 0x42143u;
    const unsigned short precisions[] = {0, 0x200, 0x300};
    for (unsigned short pc : precisions)
        for (unsigned short rc = 0; rc < 0x1000; rc += 0x400)
    {
        const unsigned short control = static_cast<unsigned short>((original & ~0xf00) | pc | rc | 0x3f);
        __asm { fldcw control }
        for (unsigned sample = 0; sample < 2048; ++sample)
        {
            std::uint32_t a, b;
            random ^= random << 13; random ^= random >> 17; random ^= random << 5;
            a = sample < 225 ? patterns[sample / 15] : random;
            random ^= random << 13; random ^= random >> 17; random ^= random << 5;
            b = sample < 225 ? patterns[sample % 15] : random;
            float left, right;
            std::memcpy(&left, &a, 4); std::memcpy(&right, &b, 4);
            unsigned short expectedStatus, actualStatus;
            __asm { fnclex }
            const bool expected = Reference(&left, &right);
            __asm { fnstsw expectedStatus }
            __asm { fnclex }
            const bool actual = Candidate(&left, &right);
            __asm { fnstsw actualStatus }
            results += expected != actual;
            // Compare C/C++ observable exception flags, not x87 condition codes.
            flags += (expectedStatus & 0x3f) != (actualStatus & 0x3f);
            ++cases;
        }
    }
    __asm { fnclex }
    __asm { fldcw original }
    std::printf("%u comparisons: %u return mismatches, %u exception-flag mismatches\n", cases, results, flags);
    return results || flags ? 1 : 0;
}
