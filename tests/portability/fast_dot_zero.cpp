#include "candidates/legacy_fast_dot_helpers.h"
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <xmmintrin.h>

#if !defined(TEST_AXIS)
#define TEST_AXIS 0
#endif

using Kernel = float (__fastcall *)(void*, void*, const void*);

__declspec(naked) float __fastcall Reference(void*, void*, const void*)
{
    __asm {
#if defined(TEST_COMPONENT)
        mov eax, dword ptr [esp+4]
        fld dword ptr [eax+TEST_AXIS*4]
#else
        fldz
#endif
        ret 4
    }
}

__declspec(noinline) float __fastcall Candidate(void*, void*, const void* vector)
{
#if defined(TEST_COMPONENT)
    return PortableFastDot::Component<TEST_AXIS>(vector);
#else
    return PortableFastDot::Zero();
#endif
}

__declspec(noinline) void Capture(Kernel kernel, const void* vector, unsigned char* output)
{
    __asm {
        push vector
        xor ecx, ecx
        xor edx, edx
        call kernel
        mov ecx, output
        fstp tbyte ptr [ecx]
    }
}

int main()
{
    const std::uint32_t patterns[] = {0, 0x80000000u, 0x3f800000u, 0xbf800000u,
        0x7f800000u, 0xff800000u, 0x7fc12345u, 0xffc54321u, 0x7f812345u,
        0xff854321u, 1, 0x80000001u, 0x007fffffu, 0x00800000u, 0x7f7fffffu};
    unsigned short original;
    __asm { fnstcw original }
    const unsigned short precisions[] = {0, 0x200, 0x300};
    std::uint32_t random = 0x781321u;
    unsigned cases = 0;
    for (unsigned short pc : precisions)
        for (unsigned short rc = 0; rc < 0x1000; rc += 0x400)
    {
        const unsigned short control = static_cast<unsigned short>((original & ~0xf00) | pc | rc | 0x3f);
        __asm { fldcw control }
        for (unsigned sample = 0; sample < 1024; ++sample)
        {
            std::uint32_t vector[3];
            for (unsigned axis = 0; axis != 3; ++axis)
            {
                random ^= random << 13; random ^= random >> 17; random ^= random << 5;
                vector[axis] = sample < 15 ? patterns[(sample + axis) % 15] : random;
            }
#if defined(TEST_COMPONENT)
            const void* input = vector;
#else
            const void* input = nullptr; // Zero must not read any component.
#endif
            unsigned char a[10], b[10];
            unsigned short expected, actual, after;
            const unsigned mxcsr = _mm_getcsr();
            __asm { fnclex }
            Capture(Reference, input, a);
            __asm { fnstsw expected }
            __asm { fnclex }
            Capture(Candidate, input, b);
            __asm { fnstsw actual }
            __asm { fnstcw after }
            if (std::memcmp(a, b, sizeof(a)) || expected != actual || after != control || mxcsr != _mm_getcsr())
            {
                std::fprintf(stderr, "Mismatch sample=%u pc=%x rc=%x status=%x/%x\n", sample, pc, rc, expected, actual);
                return 1;
            }
            ++cases;
        }
    }
    __asm { fnclex }
    __asm { fldcw original }
    std::printf("PASS: %u exact binary80 return/status comparisons and fastcall stack balance\n", cases);
    return 0;
}
