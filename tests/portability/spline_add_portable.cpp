#include "../../src/Thorn/PortableSplineMath.h"
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <initializer_list>
#include <xmmintrin.h>

__declspec(noinline) void Reference(float* dst, const float* left, const float* right)
{
    __asm {
        mov ecx, left
        mov edx, right
        mov eax, dst
        fld dword ptr [ecx+8]
#if defined(TEST_SUBTRACT_STORED)
        fsub dword ptr [edx+8]
#else
        fadd dword ptr [edx+8]
#endif
        fstp dword ptr [eax+8]
        fld dword ptr [ecx+4]
#if defined(TEST_SUBTRACT_STORED)
        fsub dword ptr [edx+4]
#else
        fadd dword ptr [edx+4]
#endif
        fstp dword ptr [eax+4]
        fld dword ptr [ecx]
#if defined(TEST_SUBTRACT_STORED)
        fsub dword ptr [edx]
#else
        fadd dword ptr [edx]
#endif
        fstp dword ptr [eax]
    }
}

__declspec(noinline) void Candidate(float* dst, const float* left, const float* right)
{
#if defined(TEST_SUBTRACT_STORED)
    ThornPortableSplineMath::SubtractStored(dst, left, right);
#else
    ThornPortableSplineMath::AddStored(dst, left, right);
#endif
}

int main()
{
    const std::uint32_t patterns[] = {0, 0x80000000u, 0x3f800000u, 0xbf800000u,
        0x7f800000u, 0xff800000u, 0x7fc12345u, 0xffc54321u, 0x7f812345u,
        0xff854321u, 1, 0x80000001u, 0x007fffffu, 0x00800000u, 0x7f7fffffu,
        0x3f000000u, 0xbf000000u, 0x40000000u, 0xffc12345u, 0x7fc54321u};
    constexpr unsigned patternCount = sizeof(patterns) / sizeof(patterns[0]);
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
            for (unsigned anchor : {48u, 96u})
            for (int shift = -12; shift <= 12; shift += 4)
        {
            alignas(float) unsigned char a[160], b[160];
            for (unsigned offset = 0; offset < sizeof(a); offset += 4)
            {
                random ^= random << 13; random ^= random >> 17; random ^= random << 5;
                const std::uint32_t bits = sample < patternCount * patternCount ?
                    patterns[(sample % patternCount + offset / 4) % patternCount] : random;
                std::memcpy(a + offset, &bits, 4);
            }
            if (sample < patternCount * patternCount)
                for (unsigned component = 0; component != 3; ++component)
                {
                    const auto bits = patterns[(sample / patternCount + component) % patternCount];
                    std::memcpy(a + 96 + component * 4, &bits, 4);
                }
            std::memcpy(b, a, sizeof(a));
            const unsigned mxcsr = _mm_getcsr();
            unsigned short expected, actual;
            __asm { fnclex }
            Reference(reinterpret_cast<float*>(a + anchor + shift), reinterpret_cast<const float*>(a + 48), reinterpret_cast<const float*>(a + 96));
            __asm { fnstsw expected }
            __asm { fnclex }
            Candidate(reinterpret_cast<float*>(b + anchor + shift), reinterpret_cast<const float*>(b + 48), reinterpret_cast<const float*>(b + 96));
            __asm { fnstsw actual }
            unsigned short afterControl;
            __asm { fnstcw afterControl }
            if (std::memcmp(a, b, sizeof(a)) || ((expected ^ actual) & 0x383f) != 0 ||
                mxcsr != _mm_getcsr() || afterControl != control)
            {
                std::fprintf(stderr, "Mismatch sample=%u shift=%d pc=%x rc=%x status=%x/%x\n", sample, shift, pc, rc, expected, actual);
                for (unsigned index = 0; index != 3; ++index)
                {
                    std::uint32_t expectedBits, actualBits;
                    std::memcpy(&expectedBits, a + anchor + shift + index * 4, 4);
                    std::memcpy(&actualBits, b + anchor + shift + index * 4, 4);
                    std::fprintf(stderr, "component=%u bits=%08x/%08x\n", index, expectedBits, actualBits);
                }
                return 1;
            }
            ++cases;
        }
    }
    __asm { fnclex }
    __asm { fldcw original }
    std::printf("PASS: %u exact binary-vector buffer/exception/stack comparisons; all precision/rounding modes\n", cases);
    return 0;
}
