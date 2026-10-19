#include "../../src/include/portable_integer_conversion.h"
#include <cstdio>
#include <cstring>
#include <xmmintrin.h>

__declspec(noinline) std::uint64_t Reference(const double* input)
{
    unsigned short saved, chop;
    std::uint64_t result;
    __asm {
        mov ecx, input
        fnstcw saved
        mov ax, saved
        or ax, 0c00h
        mov chop, ax
        fldcw chop
        fld qword ptr [ecx]
        fistp qword ptr result
        fldcw saved
    }
    return result;
}

__declspec(noinline) std::uint64_t Candidate(const double* input)
{
    return PortableIntegerConversion::ToInt64Bits(input);
}

int main()
{
    const std::uint64_t patterns[] = {0, UINT64_C(0x8000000000000000), 1,
        UINT64_C(0x8000000000000001), UINT64_C(0x000fffffffffffff),
        UINT64_C(0x0010000000000000), UINT64_C(0x3fefffffffffffff),
        UINT64_C(0x3ff0000000000000), UINT64_C(0x3ff0000000000001),
        UINT64_C(0x3ff8000000000000), UINT64_C(0xbff8000000000000),
        UINT64_C(0x43dfffffffffffff), UINT64_C(0x43e0000000000000),
        UINT64_C(0xc3e0000000000000), UINT64_C(0xc3e0000000000001),
        UINT64_C(0x7ff0000000000000), UINT64_C(0xfff0000000000000),
        UINT64_C(0x7ff8123456789abc), UINT64_C(0x7ff0123456789abc)};
    unsigned short original;
    __asm { fnstcw original }
    const unsigned short precisions[] = {0, 0x200, 0x300};
    std::uint64_t random = UINT64_C(0x48127ac319752);
    unsigned cases = 0, outputs = 0, flags = 0, controls = 0, mxcsrChanges = 0;
    for (unsigned short pc : precisions)
        for (unsigned short rc = 0; rc < 0x1000; rc += 0x400)
    {
        const unsigned short control = static_cast<unsigned short>((original & ~0xf00) | pc | rc | 0x3f);
        __asm { fldcw control }
        for (unsigned sample = 0; sample < 4096; ++sample)
        {
            random ^= random << 13; random ^= random >> 7; random ^= random << 17;
            const std::uint64_t bits = sample < sizeof(patterns) / sizeof(patterns[0]) ? patterns[sample] : random;
            double input;
            std::memcpy(&input, &bits, sizeof(input));
            unsigned short expectedStatus, actualStatus, after;
            const unsigned mxcsr = _mm_getcsr();
            __asm { fnclex }
            const auto expected = Reference(&input);
            __asm { fnstsw expectedStatus }
            __asm { fnclex }
            const auto actual = Candidate(&input);
            __asm { fnstsw actualStatus }
            __asm { fnstcw after }
            outputs += expected != actual;
            flags += (expectedStatus & 0x3f) != (actualStatus & 0x3f);
            controls += after != control;
            mxcsrChanges += mxcsr != _mm_getcsr();
            ++cases;
        }
    }
    __asm { fnclex }
    __asm { fldcw original }
    std::printf("%u comparisons: %u result differences, %u exception differences, %u control changes, %u MXCSR changes\n", cases, outputs, flags, controls, mxcsrChanges);
    return outputs || flags || controls || mxcsrChanges ? 1 : 0;
}
