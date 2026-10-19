#include "../../src/include/portable_double_load.h"
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <initializer_list>
#include <xmmintrin.h>

__declspec(noinline) void Reference(void* dst, const void* src)
{
    __asm {
        mov ecx, src
        fld qword ptr [ecx]
        mov eax, dst
        fstp qword ptr [eax]
    }
}

__declspec(noinline) void Candidate(void* dst, const void* src)
{
    PortableDoubleLoad::Copy(dst, src);
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
            for (int shift = -8; shift <= 8; ++shift)
        {
            alignas(float) unsigned char a[160], b[160];
            for (unsigned offset = 0; offset < sizeof(a); offset += 8)
            {
                random ^= random << 13; random ^= random >> 17; random ^= random << 5;
                const std::uint64_t bits = sample < 15 ? patterns[(sample + offset / 8) % 15] : random;
                std::memcpy(a + offset, &bits, 8);
            }
            std::memcpy(b, a, sizeof(a));
            const unsigned mxcsr = _mm_getcsr();
            unsigned short expected, actual;
            __asm { fnclex }
            Reference(reinterpret_cast<float*>(a + 64 + shift), reinterpret_cast<const float*>(a + 64));
            __asm { fnstsw expected }
            __asm { fnclex }
            Candidate(reinterpret_cast<float*>(b + 64 + shift), reinterpret_cast<const float*>(b + 64));
            __asm { fnstsw actual }
            if (std::memcmp(a, b, sizeof(a)) || expected != actual || mxcsr != _mm_getcsr())
            {
                std::fprintf(stderr, "Mismatch sample=%u shift=%d pc=%x rc=%x status=%x/%x\n", sample, shift, pc, rc, expected, actual);
                return 1;
            }
            ++cases;
        }
    }
    __asm { fnclex }
    __asm { fldcw original }
    std::printf("PASS: %u exact double-copy buffer/status comparisons; all precision/rounding modes\n", cases);
    return 0;
}
