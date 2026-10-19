#include "../../src/Thorn/PortableSplineMath.h"
#include "../../src/include/portable_vector_load.h"
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <initializer_list>
#include <xmmintrin.h>

__declspec(noinline) void Reference(float* dst, const float* src)
{
    __asm {
        mov ecx, src
#if defined(TEST_COLUMN_COPY)
        fld dword ptr [ecx+24]
#else
        fld dword ptr [ecx+8]
#endif
        mov eax, dst
#if !defined(TEST_VECTOR_COPY)
        fchs
#endif
#if defined(TEST_COLUMN_COPY)
        fld dword ptr [ecx+12]
#else
        fld dword ptr [ecx+4]
#endif
#if !defined(TEST_VECTOR_COPY)
        fchs
#endif
        fld dword ptr [ecx]
#if !defined(TEST_VECTOR_COPY)
        fchs
#endif
        fstp dword ptr [eax]
        fstp dword ptr [eax+4]
        fstp dword ptr [eax+8]
    }
}

__declspec(noinline) void Candidate(float* dst, const float* src)
{
#if defined(TEST_COLUMN_COPY)
    PortableVectorLoad::Copy(dst, src, 3);
#elif defined(TEST_VECTOR_COPY)
    PortableVectorLoad::Copy(dst, src);
#else
    ThornPortableSplineMath::NegateVector(dst, src);
#endif
}

int main()
{
    const std::uint32_t patterns[] = {0, 0x80000000u, 0x3f800000u, 0xbf800000u,
        0x7f800000u, 0xff800000u, 0x7fc12345u, 0xffc54321u, 0x7f812345u,
        0xff854321u, 1, 0x80000001u, 0x007fffffu, 0x00800000u, 0x7f7fffffu};
    unsigned short original;
    __asm { fnstcw original }
    std::uint32_t random = 0xb34217u;
    unsigned cases = 0;
    for (unsigned short pc : {0, 0x200, 0x300})
        for (unsigned short rc = 0; rc < 0x1000; rc += 0x400)
    {
        const unsigned short control = static_cast<unsigned short>((original & ~0xf00) | pc | rc | 0x3f);
        __asm { fldcw control }
        for (unsigned sample = 0; sample < 512; ++sample)
            for (int shift = -12; shift <= 12; shift += 4)
        {
            alignas(float) unsigned char a[64], b[64];
            for (unsigned offset = 0; offset < sizeof(a); offset += 4)
            {
                random ^= random << 13; random ^= random >> 17; random ^= random << 5;
                const std::uint32_t bits = sample < 15 ? patterns[(sample + offset / 4) % 15] : random;
                std::memcpy(a + offset, &bits, 4);
            }
            std::memcpy(b, a, sizeof(a));
            const unsigned mxcsr = _mm_getcsr();
            unsigned short expected, actual;
            __asm { fnclex }
            Reference(reinterpret_cast<float*>(a + 24 + shift), reinterpret_cast<const float*>(a + 24));
            __asm { fnstsw expected }
            __asm { fnclex }
            Candidate(reinterpret_cast<float*>(b + 24 + shift), reinterpret_cast<const float*>(b + 24));
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
    std::printf("PASS: %u exact vector buffer/status comparisons; all precision/rounding modes\n", cases);
    return 0;
}
