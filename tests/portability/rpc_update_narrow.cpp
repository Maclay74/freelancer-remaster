#include "../../src/include/portable_rpc_update_fields.h"
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <xmmintrin.h>

__declspec(noinline) static void Reference(void* destination, const void* source)
{
    __asm {
        mov eax, source
        fld qword ptr [eax]
        mov eax, destination
        fstp dword ptr [eax]
    }
}

__declspec(noinline) static void Candidate(void* destination, const void* source)
{
    PortableRpcUpdateFields::NarrowTimestamp(destination, source);
}

int main()
{
    const std::uint64_t special[] = {0, UINT64_C(0x8000000000000000),
        UINT64_C(0x3ff0000000000000), UINT64_C(0xbff0000000000000),
        UINT64_C(0x7ff0000000000000), UINT64_C(0xfff0000000000000),
        UINT64_C(0x7ff8123456789abc), UINT64_C(0xfff8123456789abc),
        UINT64_C(0x7ff0123456789abc), 1, UINT64_C(0x8000000000000001),
        UINT64_C(0x36a0000000000000), UINT64_C(0x3690000000000000),
        UINT64_C(0x3810000000000000), UINT64_C(0x380fffffc0000000),
        UINT64_C(0x47efffffe0000000), UINT64_C(0x47effffff0000000),
        UINT64_C(0x7fefffffffffffff), UINT64_C(0x0010000000000000)};
    unsigned short original;
    __asm { fnstcw original }
    const unsigned originalMxcsr = _mm_getcsr();
    const unsigned short precision[] = {0, 0x200, 0x300};
    std::uint64_t random = UINT64_C(0xb42196c3);
    unsigned cases = 0;
    for (auto pc : precision) for (unsigned short rc = 0; rc < 0x1000; rc += 0x400)
    {
        const unsigned short control = static_cast<unsigned short>((original & ~0xf00) | pc | rc | 0x3f);
        __asm { fldcw control }
        for (unsigned sample = 0; sample < 512; ++sample) for (int shift = -3; shift <= 3; ++shift)
        {
            random ^= random << 13; random ^= random >> 7; random ^= random << 17;
            const auto bits = sample < sizeof(special) / sizeof(special[0]) ? special[sample] : random;
            unsigned char expected[64], actual[64];
            std::memset(expected, 0x5a, sizeof(expected));
            std::memcpy(expected + 24, &bits, sizeof(bits));
            std::memcpy(actual, expected, sizeof(actual));
            unsigned short oldStatus, newStatus, after;
            __asm { fnclex }
            Reference(expected + 24 + shift, expected + 24);
            __asm { fnstsw oldStatus }
            __asm { fnclex }
            Candidate(actual + 24 + shift, actual + 24);
            __asm { fnstsw newStatus }
            __asm { fnstcw after }
            if (std::memcmp(expected, actual, sizeof(actual)) || ((oldStatus ^ newStatus) & 0x383f) ||
                after != control || originalMxcsr != _mm_getcsr())
            {
                std::fprintf(stderr, "Mismatch sample=%u shift=%d pc=%x rc=%x bits=%016llx status=%x/%x\n",
                    sample, shift, pc, rc, static_cast<unsigned long long>(bits), oldStatus, newStatus);
                return 1;
            }
            ++cases;
        }
    }
    __asm { fnclex }
    __asm { fldcw original }
    std::printf("PASS: %u exact timestamp buffer/exception/stack comparisons; controls/MXCSR unchanged\n", cases);
    return 0;
}
