#include "../../src/include/portable_vector_load.h"
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <initializer_list>
#include <xmmintrin.h>

__declspec(noinline) void Reference(float* output, const float* xy, const double* z)
{
    __asm {
        mov eax, z
        fld qword ptr [eax]
        mov eax, xy
        fld dword ptr [eax]
        mov edx, output
        mov ecx, dword ptr [eax+4]
        fstp dword ptr [edx]
        fstp dword ptr [edx+8]
        mov dword ptr [edx+4], ecx
    }
}

__declspec(noinline) void Candidate(float* output, const float* xy, const double* z)
{
    PortableVectorLoad::CopyMixed(output, xy, z);
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
    const std::uint32_t floats[] = {0, 0x80000000u, 0x3f000000u, 0x3f800000u,
        0xbf800000u, 0x7f800000u, 0xff800000u, 0x7fc12345u, 0x7f812345u,
        0xff854321u, 1, 0x80000001u, 0x007fffffu, 0x00800000u, 0x7f7fffffu};
    unsigned short original;
    __asm { fnstcw original }
    std::uint64_t random = 0xb34217u;
    unsigned cases = 0;
    for (unsigned short pc : {0, 0x200, 0x300})
        for (unsigned short rc = 0; rc < 0x1000; rc += 0x400)
    {
        const unsigned short control = static_cast<unsigned short>((original & ~0xf00) | pc | rc | 0x3f);
        __asm { fldcw control }
        for (unsigned sample = 0; sample < 512; ++sample)
            for (int shift = -12; shift <= 24; shift += 4)
        {
            alignas(float) unsigned char a[160], b[160];
            for (unsigned offset = 0; offset < sizeof(a); offset += 8)
            {
                random ^= random << 13; random ^= random >> 17; random ^= random << 5;
                const std::uint64_t bits = sample < 15 ? patterns[(sample + offset / 8) % 15] : random;
                std::memcpy(a + offset, &bits, 8);
            }
            const auto xBits = sample < 225 ? floats[sample / 15] : static_cast<std::uint32_t>(random);
            const auto yBits = floats[sample % 15];
            auto zBits = sample < 225 ? patterns[sample % 15] : random;
            const std::uint64_t edges[] = {UINT64_C(0x3690000000000000),
                UINT64_C(0x3690000000000001), UINT64_C(0xb690000000000000),
                UINT64_C(0x36a0000000000000), UINT64_C(0x380fffffffffffff),
                UINT64_C(0x3810000000000000), UINT64_C(0x47efffffe0000000),
                UINT64_C(0x47f0000000000000), UINT64_C(0xc7f0000000000000)};
            if (sample >= 225 && sample < 225 + sizeof(edges) / sizeof(edges[0]))
                zBits = edges[sample - 225];
            std::memcpy(a + 48, &xBits, 4);
            std::memcpy(a + 52, &yBits, 4);
            std::memcpy(a + 64, &zBits, 8);
            std::memcpy(b, a, sizeof(a));
            const unsigned mxcsr = _mm_getcsr();
            unsigned short expected, actual;
            __asm { fnclex }
            Reference(reinterpret_cast<float*>(a + 48 + shift), reinterpret_cast<const float*>(a + 48), reinterpret_cast<const double*>(a + 64));
            __asm { fnstsw expected }
            __asm { fnclex }
            Candidate(reinterpret_cast<float*>(b + 48 + shift), reinterpret_cast<const float*>(b + 48), reinterpret_cast<const double*>(b + 64));
            __asm { fnstsw actual }
            unsigned short afterControl;
            __asm { fnstcw afterControl }
            if (std::memcmp(a, b, sizeof(a)) || ((expected ^ actual) & 0x383f) != 0 ||
                mxcsr != _mm_getcsr() || afterControl != control)
            {
                std::fprintf(stderr, "Mismatch sample=%u shift=%d pc=%x rc=%x status=%x/%x\n", sample, shift, pc, rc, expected, actual);
                return 1;
            }
            ++cases;
        }
    }
    __asm { fnclex }
    __asm { fldcw original }
    std::printf("PASS: %u exact mixed-vector buffer/exception/stack comparisons; all precision/rounding modes\n", cases);
    return 0;
}
