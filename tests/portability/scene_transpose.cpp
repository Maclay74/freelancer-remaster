#include "../../src/Thorn/PortableSceneMath.h"
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <initializer_list>
#include <xmmintrin.h>

__declspec(noinline) void Reference(float* dst, const float* src)
{
    unsigned int raw;
    __asm {
        mov ecx, src
        fld dword ptr [ecx+20h]
        mov eax, dword ptr [ecx+0ch]
        fld dword ptr [ecx+14h]
        mov raw, eax
        fld dword ptr [ecx+8]
        mov eax, dst
        fld dword ptr [ecx+1ch]
        fld dword ptr [ecx+10h]
        fld dword ptr [ecx+4]
        fld dword ptr [ecx+18h]
        fld dword ptr [ecx]
        mov ecx, raw
        fstp dword ptr [eax]
        mov dword ptr [eax+4], ecx
        fstp dword ptr [eax+8]
        fstp dword ptr [eax+0ch]
        fstp dword ptr [eax+10h]
        fstp dword ptr [eax+14h]
        fstp dword ptr [eax+18h]
        fstp dword ptr [eax+1ch]
        fstp dword ptr [eax+20h]
    }
}

__declspec(noinline) void Candidate(float* dst, const float* src)
{
    ThornPortableSceneMath::Transpose9(dst, src);
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
            for (int shift = -36; shift <= 36; shift += 4)
        {
            alignas(float) unsigned char a[160], b[160];
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
    std::printf("PASS: %u exact transpose buffer/status comparisons; all precision/rounding modes\n", cases);
    return 0;
}
