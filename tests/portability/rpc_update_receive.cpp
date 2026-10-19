#include "../../src/include/portable_rpc_update_fields.h"
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <xmmintrin.h>

__declspec(noinline) static void Reference(void* timeDestination, void* fieldDestination,
                                          const void* timeSource, const void* fieldSource)
{
    float timeRounded, fieldRounded;
    __asm {
        mov eax, timeSource
        fld dword ptr [eax]
        fst dword ptr timeRounded
        mov eax, fieldSource
        fld dword ptr [eax]
        fst dword ptr fieldRounded
        fxch st(1)
        mov eax, timeDestination
        fstp dword ptr [eax]
        mov eax, fieldDestination
        fstp dword ptr [eax]
    }
}

__declspec(noinline) static void Candidate(void* timeDestination, void* fieldDestination,
                                          const void* timeSource, const void* fieldSource)
{
    PortableRpcUpdateFields::CopyReceived(timeDestination, fieldDestination, timeSource, fieldSource);
}

int main()
{
    const std::uint32_t patterns[] = {0, 0x80000000u, 0x3f800000u, 0xbf800000u,
        0x7f800000u, 0xff800000u, 0x7fc12345u, 0xffc54321u, 0x7f812345u,
        0xff854321u, 1, 0x80000001u, 0x007fffffu, 0x00800000u, 0x7f7fffffu};
    const unsigned destinations[][2] = {{80, 84}, {16, 20}, {48, 52}, {48, 16}, {80, 80}, {17, 19}};
    const unsigned short precision[] = {0, 0x200, 0x300};
    unsigned short original;
    __asm { fnstcw original }
    const unsigned originalMxcsr = _mm_getcsr();
    std::uint32_t random = 0x46b12u;
    unsigned cases = 0;
    for (auto pc : precision) for (unsigned short rc = 0; rc < 0x1000; rc += 0x400)
    {
        const unsigned short control = static_cast<unsigned short>((original & ~0xf00) | pc | rc | 0x3f);
        __asm { fldcw control }
        for (unsigned sample = 0; sample < 512; ++sample) for (const auto& offsets : destinations)
        {
            unsigned char expected[112], actual[112];
            std::memset(expected, 0xa5, sizeof(expected));
            for (unsigned input = 0; input < 2; ++input)
            {
                random ^= random << 13; random ^= random >> 17; random ^= random << 5;
                const unsigned count = sizeof(patterns) / sizeof(patterns[0]);
                const auto bits = sample < count * count ? patterns[input ? sample / count : sample % count] : random;
                std::memcpy(expected + (input ? 48 : 16), &bits, sizeof(bits));
            }
            std::memcpy(actual, expected, sizeof(actual));
            unsigned short oldStatus, newStatus, after;
            __asm { fnclex }
            Reference(expected + offsets[0], expected + offsets[1], expected + 16, expected + 48);
            __asm { fnstsw oldStatus }
            __asm { fnclex }
            Candidate(actual + offsets[0], actual + offsets[1], actual + 16, actual + 48);
            __asm { fnstsw newStatus }
            __asm { fnstcw after }
            if (std::memcmp(expected, actual, sizeof(actual)) || ((oldStatus ^ newStatus) & 0x383f) ||
                after != control || originalMxcsr != _mm_getcsr())
            {
                std::fprintf(stderr, "Mismatch sample=%u dest=%u/%u pc=%x rc=%x status=%x/%x\n",
                    sample, offsets[0], offsets[1], pc, rc, oldStatus, newStatus);
                return 1;
            }
            ++cases;
        }
    }
    __asm { fnclex }
    __asm { fldcw original }
    std::printf("PASS: %u exact received-field buffer/exception/stack comparisons; controls/MXCSR unchanged\n", cases);
    return 0;
}
