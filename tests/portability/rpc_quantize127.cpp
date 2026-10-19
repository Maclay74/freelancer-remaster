#include "../../src/include/portable_rpc_quantization.h"
#if defined(TEST_QUANTIZE255)
static const double Scale = 255.0;
#else
static const double Scale = 127.0;
#endif
#if defined(TEST_UPDATE_COMMIT)
static unsigned int* PointerToCommit;
static unsigned int CommittedPointer;
#endif
#include <cstdio>
#include <cstring>
#include <xmmintrin.h>

__declspec(noinline) std::uint64_t Reference(const float* input)
{
    unsigned short saved, chop;
    std::uint64_t result;
    __asm {
        mov ecx, input
        fld dword ptr [ecx]
        fmul qword ptr Scale
#if defined(TEST_UPDATE_COMMIT)
        mov ecx, PointerToCommit
        test ecx, ecx
        jz Conversion
        mov eax, CommittedPointer
        mov dword ptr [ecx], eax
    Conversion:
#endif
        fnstcw saved
        mov ax, saved
        or ax, 0c00h
        mov chop, ax
        fldcw chop
        fistp qword ptr result
        fldcw saved
    }
    return result;
}

__declspec(noinline) std::uint64_t Candidate(const float* input)
{
#if defined(TEST_UPDATE_COMMIT)
    return PortableRpcQuantization::QuantizeUpdate(input, PointerToCommit, CommittedPointer);
#else
    return PortableRpcQuantization::QuantizeByte(input, Scale);
#endif
}

int main()
{
    const std::uint32_t patterns[] = {0, 0x80000000u, 1, 0x80000001u,
        0x007fffffu, 0x00800000u, 0x3f7fffffu, 0x3f800000u, 0x3f800001u,
        0x3f000000u, 0xbf000000u, 0x5b800000u, 0xdb800000u, 0x7f7fffffu,
        0xff7fffffu, 0x7f800000u, 0xff800000u, 0x7fc12345u, 0x7f812345u,
        0x3c010203u, 0xbc010203u, 0x3c010204u, 0xbc010204u, 0x3c010205u, 0xbc010205u, 0x5b810203u, 0xdb810203u, 0x5b810204u, 0xdb810204u, 0x5b810205u, 0xdb810205u, 0x3b808080u, 0xbb808080u, 0x3b808081u, 0xbb808081u, 0x3b808082u, 0xbb808082u, 0x5b008080u, 0xdb008080u, 0x5b008081u, 0xdb008081u, 0x5b008082u, 0xdb008082u};
    unsigned short original;
    __asm { fnstcw original }
    const unsigned short precisions[] = {0, 0x200, 0x300};
    std::uint32_t random = 0x48127acu;
    unsigned cases = 0, outputs = 0, flags = 0, controls = 0, mxcsrChanges = 0;
    for (unsigned short pc : precisions)
        for (unsigned short rc = 0; rc < 0x1000; rc += 0x400)
    {
        const unsigned short control = static_cast<unsigned short>((original & ~0xf00) | pc | rc | 0x3f);
        __asm { fldcw control }
        for (unsigned sample = 0; sample < 4096; ++sample)
        {
            random ^= random << 13; random ^= random >> 7; random ^= random << 17;
            const std::uint32_t bits = sample < sizeof(patterns) / sizeof(patterns[0]) ? patterns[sample] : random;
            float input;
            std::memcpy(&input, &bits, sizeof(input));
            unsigned short expectedStatus, actualStatus, after;
            const unsigned mxcsr = _mm_getcsr();
            __asm { fnclex }
#if defined(TEST_UPDATE_COMMIT)
            unsigned int expectedCommit = 0xa5a5a5a5u, actualCommit = expectedCommit;
            CommittedPointer = bits;
            PointerToCommit = sample & 1 ? &expectedCommit : nullptr;
#endif
            const auto expected = Reference(&input);
            __asm { fnstsw expectedStatus }
            __asm { fnclex }
#if defined(TEST_UPDATE_COMMIT)
            PointerToCommit = sample & 1 ? &actualCommit : nullptr;
#endif
            const auto actual = Candidate(&input);
            __asm { fnstsw actualStatus }
            __asm { fnstcw after }
            outputs += expected != actual;
#if defined(TEST_UPDATE_COMMIT)
            outputs += expectedCommit != actualCommit;
#endif
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
