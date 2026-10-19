#include "../../src/Thorn/PortableComparison.h"
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <xmmintrin.h>

__declspec(noinline) bool Reference(const double* value)
{
    const double zero = 0.0;
    unsigned char nonzero;
    __asm {
        mov ecx, value
        fld qword ptr [ecx]
        fcomp qword ptr zero
        fnstsw ax
        test ah, 44h
        setp nonzero
    }
    return nonzero != 0;
}

__declspec(noinline) bool Candidate(const double* value)
{
    return ThornPortableComparison::Nonzero(value);
}

int main()
{
    const std::uint64_t patterns[] = {0, UINT64_C(0x8000000000000000),
        UINT64_C(0x3ff0000000000000), UINT64_C(0xbff0000000000000),
        UINT64_C(0x7ff0000000000000), UINT64_C(0xfff0000000000000),
        UINT64_C(0x7ff8123456789abc), UINT64_C(0xfff8543211234567),
        UINT64_C(0x7ff0123456789abc), UINT64_C(0xfff0543211234567),
        1, UINT64_C(0x8000000000000001), UINT64_C(0x000fffffffffffff),
        UINT64_C(0x0010000000000000), UINT64_C(0x7fefffffffffffff)};
    constexpr unsigned patternCount = sizeof(patterns) / sizeof(patterns[0]);
    unsigned short original;
    __asm { fnstcw original }
    unsigned cases = 0, results = 0, flags = 0, mxcsrDifferences = 0;
    std::uint64_t random = 0x42143u;
    const unsigned short precisions[] = {0, 0x200, 0x300};
    for (unsigned short pc : precisions)
        for (unsigned short rc = 0; rc < 0x1000; rc += 0x400)
    {
        const unsigned short control = static_cast<unsigned short>((original & ~0xf00) | pc | rc | 0x3f);
        __asm { fldcw control }
        for (unsigned sample = 0; sample < 2048; ++sample)
        {
            std::uint64_t a, b;
            random ^= random << 13; random ^= random >> 17; random ^= random << 5;
            a = sample < patternCount * patternCount ? patterns[sample / patternCount] : random;
            random ^= random << 13; random ^= random >> 17; random ^= random << 5;
            b = sample < patternCount * patternCount ? patterns[sample % patternCount] : random;
            double left, right;
            std::memcpy(&left, &a, 8); std::memcpy(&right, &b, 8);
            unsigned short expectedStatus, actualStatus;
            __asm { fnclex }
            const bool expected = Reference(&left);
            __asm { fnstsw expectedStatus }
            __asm { fnclex }
            const unsigned beforeMxcsr = _mm_getcsr();
            const bool actual = Candidate(&left);
            __asm { fnstsw actualStatus }
            mxcsrDifferences += beforeMxcsr != _mm_getcsr();
            results += expected != actual;
            // Compare C/C++ observable exception flags, not x87 condition codes.
            flags += (expectedStatus & 0x3f) != (actualStatus & 0x3f);
            ++cases;
        }
    }
    __asm { fnclex }
    __asm { fldcw original }
    std::printf("%u comparisons: %u return mismatches, %u exception-flag mismatches, %u MXCSR changes\n", cases, results, flags, mxcsrDifferences);
    return results || flags || mxcsrDifferences ? 1 : 0;
}
