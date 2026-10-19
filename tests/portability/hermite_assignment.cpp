#include "../../src/Thorn/HermiteRecord.h"
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <xmmintrin.h>

using ThornCurveDetail::FHermiteKnot;

// Retained original kernel as an x86-only oracle; no production assembly.
__declspec(noinline) void Reference(FHermiteKnot* Destination, const FHermiteKnot* Source)
{
    __asm {
        mov eax, Destination
        mov ecx, Source
        mov edx, dword ptr [ecx]
        mov dword ptr [eax], edx
        mov edx, dword ptr [ecx + 4]
        mov dword ptr [eax + 4], edx
        mov edx, dword ptr [ecx + 8]
        mov dword ptr [eax + 8], edx
        mov edx, dword ptr [ecx + 0ch]
        mov dword ptr [eax + 0ch], edx
    }
}

__declspec(noinline) void Candidate(FHermiteKnot* dst, const FHermiteKnot* src)
{
    ThornCurveDetail::AssignHermiteRecord(dst, src);
}

int main()
{
    const std::uint32_t patterns[] = {0, 0x80000000u, 0x3f800000u, 0x7f800000u,
        0xff800000u, 0x7fc12345u, 0x7f812345u, 1, 0x007fffffu, 0x00800000u, 0x7f7fffffu};
    unsigned short original;
    __asm { fnstcw original }
    const unsigned short precisions[] = {0, 0x200, 0x300};
    unsigned cases = 0;
    std::uint32_t random = 0xa932741bu;
    for (unsigned short precision : precisions) for (unsigned short rounding = 0; rounding < 0x1000; rounding += 0x400)
    {
        const unsigned short control = static_cast<unsigned short>((original & ~0xf00) | precision | rounding);
        __asm { fldcw control }
        for (unsigned sample = 0; sample < 10000; ++sample)
        {
            // Source/destination range from disjoint through self and partial
            // overlap. The byte backing storage is accessed only through bytes.
            for (int shift = -16; shift <= 16; shift += 4)
            {
                alignas(FHermiteKnot) unsigned char a[64], b[64];
                for (unsigned offset = 0; offset < sizeof(a); offset += 4)
                {
                    random ^= random << 13; random ^= random >> 17; random ^= random << 5;
                    const auto bits = sample < 11 ? patterns[(sample + offset / 4) % 11] : random;
                    std::memcpy(a + offset, &bits, sizeof(bits));
                }
                std::memcpy(b, a, sizeof(a));
                unsigned short before, after;
                const unsigned mxcsr = _mm_getcsr();
                __asm { fnstsw before }
                Reference(reinterpret_cast<FHermiteKnot*>(a + 24 + shift), reinterpret_cast<FHermiteKnot*>(a + 24));
                Candidate(reinterpret_cast<FHermiteKnot*>(b + 24 + shift), reinterpret_cast<FHermiteKnot*>(b + 24));
                __asm { fnstsw after }
                if (std::memcmp(a, b, sizeof(a)) || before != after || mxcsr != _mm_getcsr())
                {
                    std::fprintf(stderr, "Mismatch: sample=%u shift=%d pc=%x rounding=%x\n", sample, shift, precision, rounding);
                    return 1;
                }
                ++cases;
            }
        }
    }
    __asm { fldcw original }
    std::printf("PASS: %u exact buffer comparisons; x87/MXCSR unchanged; all precision/rounding modes\n", cases);
    return 0;
}
