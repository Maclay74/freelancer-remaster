#include "../../src/Thorn/ExtendedFloatStorage.h"
#if defined(TEST_FLOAT_COPY)
#include "../../src/include/portable_float_load.h"
#endif
#include <windows.h>
#include <xmmintrin.h>
#include <cstdio>
#include <initializer_list>

__declspec(noinline) void Reference(unsigned char* output, const float* value)
{
    __asm {
        mov ecx, value
        fld dword ptr [ecx]
        mov ecx, output
#if defined(TEST_FLOAT_COPY)
        fstp dword ptr [ecx]
#else
        fstp tbyte ptr [ecx]
#endif
    }
}
__declspec(noinline) void Candidate(unsigned char* output, const float* value)
{
#if defined(TEST_FLOAT_COPY)
    PortableFloatLoad::Copy(output, value);
#else
    ThornPortableMath::StoreFloatToExtended80(output, value);
#endif
}
__declspec(noinline) void ClearFlags()
{
    __asm {
        fldz
        fstp st(0)
        fnclex
    }
}
using Kernel = void (*)(unsigned char*, const float*);
static int CaptureException(EXCEPTION_POINTERS* information, unsigned short* status)
{
    *status = static_cast<unsigned short>(information->ContextRecord->FloatSave.StatusWord);
    __asm { fnclex }
    return EXCEPTION_EXECUTE_HANDLER;
}
static unsigned RunUnmasked(Kernel kernel, unsigned char* out, const float* value,
                            unsigned short control, unsigned short* status)
{
    unsigned code = 0;
    ClearFlags();
    __asm { fldcw control }
    __try {
        kernel(out, value);
        unsigned short completed;
        __asm { fnstsw completed }
        *status = completed;
    }
    __except (CaptureException(GetExceptionInformation(), status)) { code = GetExceptionCode(); }
    __asm { fninit }
    return code;
}

int main()
{
    unsigned short original;
    __asm { fnstcw original }
    const unsigned short precisions[] = {0, 0x200, 0x300};
    const std::uint32_t patterns[] = {0, 0x80000000u, 0x3f800000u, 0xbf800000u,
        0x7f800000u, 0xff800000u, 0x7fc12345u, 0xffc54321u, 0x7f812345u, 0xff854321u,
        1, 0x80000001u, 0x007fffffu, 0x00800000u, 0x7f7fffffu};
    unsigned cases = 0;
    std::uint32_t random = 0xab12734;
    for (unsigned short pc : precisions) for (unsigned short rc = 0; rc < 0x1000; rc += 0x400)
    {
        const unsigned short control = static_cast<unsigned short>((original & ~0xf00) | pc | rc | 0x3f);
        __asm { fldcw control }
        for (unsigned sample = 0; sample < 10000; ++sample) for (int shift = -10; shift <= 10; ++shift)
        {
            alignas(float) unsigned char a[40], b[40];
            std::memset(a, 0x5a, sizeof(a));
            random ^= random << 13; random ^= random >> 17; random ^= random << 5;
            const std::uint32_t bits = sample < 15 ? patterns[sample] : random;
            std::memcpy(a + 16, &bits, sizeof(bits));
            std::memcpy(b, a, sizeof(a));
            ClearFlags();
            const unsigned mxcsr = _mm_getcsr();
            unsigned short expected, actual;
            Reference(a + 16 + shift, reinterpret_cast<const float*>(a + 16));
            __asm { fnstsw expected }
            ClearFlags();
            Candidate(b + 16 + shift, reinterpret_cast<const float*>(b + 16));
            __asm { fnstsw actual }
            if (std::memcmp(a, b, sizeof(a)) || expected != actual || mxcsr != _mm_getcsr())
            {
                std::fprintf(stderr, "Mismatch bits=%08x shift=%d pc=%x rc=%x status=%x/%x\n", bits, shift, pc, rc, expected, actual);
                return 1;
            }
            ++cases;
        }
    }
    unsigned exceptional = 0;
    for (unsigned short pc : precisions) for (unsigned short rc = 0; rc < 0x1000; rc += 0x400)
        for (unsigned mask : {1u, 2u}) for (int shift = -10; shift <= 10; ++shift)
        {
            alignas(float) unsigned char a[40], b[40];
            std::memset(a, 0x5a, sizeof(a));
            const std::uint32_t bits = mask == 1 ? 0x7f812345u : 1u;
            std::memcpy(a + 16, &bits, sizeof(bits));
            std::memcpy(b, a, sizeof(a));
            const unsigned short control = static_cast<unsigned short>(((original & ~0xf00) | pc | rc | 0x3f) & ~mask);
            unsigned short expectedStatus, actualStatus;
            const auto expected = RunUnmasked(Reference, a + 16 + shift, reinterpret_cast<const float*>(a + 16), control, &expectedStatus);
            const auto actual = RunUnmasked(Candidate, b + 16 + shift, reinterpret_cast<const float*>(b + 16), control, &actualStatus);
            if (expected != actual || expectedStatus != actualStatus || std::memcmp(a, b, sizeof(a)))
            {
                std::fprintf(stderr, "Unmasked mismatch mask=%u shift=%d codes=%x/%x status=%x/%x\n", mask, shift, expected, actual, expectedStatus, actualStatus);
                return 2;
            }
            ++exceptional;
        }
    ClearFlags();
    __asm { fldcw original }
    std::printf("PASS: %u exact encoding/buffer/status comparisons and %u unmasked cases\n", cases, exceptional);
    return 0;
}
