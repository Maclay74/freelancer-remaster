#include "../../src/include/portable_integer_conversion.h"
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <initializer_list>
#include <xmmintrin.h>

__declspec(noinline) void Reference(float* dst, const float* src)
{
    const float* Source = src;
    void* Output = dst;
#if defined(TEST_AMBIENT_STORE)

        unsigned short OldControl;
        unsigned short ChopControl;
        long long Result;
        __asm {
            mov ecx, Source
            fld dword ptr [ecx + 8]
            fld dword ptr [ecx]
            fwait
            fnstcw OldControl
            fwait
            mov ax, OldControl
            or ax, 0c00h
            mov ChopControl, ax
            fldcw ChopControl
            fistp qword ptr Result
            fldcw OldControl
            fld dword ptr [ecx + 4]
            mov eax, dword ptr Result
            mov edx, Output
            mov dword ptr [edx], eax
            fwait
            fnstcw OldControl
            fwait
            mov ax, OldControl
            or ax, 0c00h
            mov ChopControl, ax
            fldcw ChopControl
            fistp qword ptr Result
            fldcw OldControl
            mov eax, dword ptr Result
            mov dword ptr [edx + 4], eax
            fwait
            fnstcw OldControl
            fwait
            mov ax, OldControl
            or ax, 0c00h
            mov ChopControl, ax
            fldcw ChopControl
            fistp qword ptr Result
            fldcw OldControl
            mov eax, dword ptr Result
            mov dword ptr [edx + 8], eax
        }
#else

        unsigned short OldControl;
        unsigned short ChopControl;
        long long Result;
        __asm {
            mov ecx, Source
            fld dword ptr [ecx]
            fwait
            fnstcw OldControl
            fwait
            mov ax, OldControl
            or ax, 0c00h
            mov ChopControl, ax
            fldcw ChopControl
            fistp qword ptr Result
            fldcw OldControl
            fld dword ptr [ecx + 4]
            mov eax, dword ptr Result
            mov edx, Output
            mov dword ptr [edx], eax
            fwait
            fnstcw OldControl
            fwait
            mov ax, OldControl
            or ax, 0c00h
            mov ChopControl, ax
            fldcw ChopControl
            fistp qword ptr Result
            fldcw OldControl
            fld dword ptr [ecx + 8]
            mov eax, dword ptr Result
            mov dword ptr [edx + 4], eax
            fwait
            fnstcw OldControl
            fwait
            mov ax, OldControl
            or ax, 0c00h
            mov ChopControl, ax
            fldcw ChopControl
            fistp qword ptr Result
            fldcw OldControl
            mov eax, dword ptr Result
            mov dword ptr [edx + 8], eax
        }
#endif
}

__declspec(noinline) void Candidate(float* dst, const float* src)
{
#if defined(TEST_AMBIENT_STORE)
    PortableIntegerConversion::StoreFloatTriplet(src, dst, true);
#else
    PortableIntegerConversion::StoreFloatTriplet(src, dst, false);
#endif
}

int main()
{
    const std::uint32_t patterns[] = {0, 0x80000000u, 0x3f800000u, 0xbf800000u,
        0x7f800000u, 0xff800000u, 0x7fc12345u, 0xffc54321u, 0x7f812345u,
        0xff854321u, 0x3f000000u, 0x3fc00000u, 0xbfc00000u, 0x5f000000u, 0xdf000000u, 1, 0x80000001u, 0x007fffffu, 0x00800000u, 0x7f7fffffu};
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
            for (int shift = -12; shift <= 12; shift += 4)
        {
            alignas(float) unsigned char a[160], b[160];
            for (unsigned offset = 0; offset < sizeof(a); offset += 4)
            {
                random ^= random << 13; random ^= random >> 17; random ^= random << 5;
                const std::uint32_t bits = sample < patternCount ? patterns[(sample + offset / 4) % patternCount] : random;
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
            if (std::memcmp(a, b, sizeof(a)) || ((expected ^ actual) & 0x383f) != 0 || mxcsr != _mm_getcsr())
            {
                std::fprintf(stderr, "Mismatch sample=%u shift=%d pc=%x rc=%x status=%x/%x\n", sample, shift, pc, rc, expected, actual);
                return 1;
            }
            ++cases;
        }
    }
    __asm { fnclex }
    __asm { fldcw original }
    std::printf("PASS: %u exact integer-triplet buffer/exception/stack comparisons; all precision/rounding modes\n", cases);
    return 0;
}
