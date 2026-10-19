#include "../../src/Thorn/PortableSplineMath.h"
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <xmmintrin.h>
#include <windows.h>
#include <initializer_list>

__declspec(noinline) void Reference(float* Dst, const float* Src)
{
    __asm {
        mov ecx, Src
        mov eax, Dst
        fld dword ptr [ecx + 8]
        fchs
        fstp dword ptr [eax + 8]
        fld dword ptr [ecx + 4]
        fchs
        fstp dword ptr [eax + 4]
        fld dword ptr [ecx]
        fchs
        fstp dword ptr [eax]
    }
}

__declspec(noinline) void Candidate(float* dst, const float* src)
{
    ThornPortableSplineMath::NegateStored(dst, src);
}

__declspec(noinline) void ClearFlags()
{
    __asm {
        fldz
        fstp st(0)
        fnclex
    }
}

using Kernel = void (*)(float*, const float*);
static int CaptureException(EXCEPTION_POINTERS* information, unsigned short* status)
{
    *status = static_cast<unsigned short>(information->ContextRecord->FloatSave.StatusWord);
    __asm { fnclex }
    return EXCEPTION_EXECUTE_HANDLER;
}

// Keep SEH outside functions with C++ objects requiring destruction.
static unsigned RunUnmasked(Kernel kernel, float* dst, const float* src,
                            unsigned short control, unsigned short* status)
{
    unsigned code = 0;
    ClearFlags();
    __asm { fldcw control }
    __try {
        kernel(dst, src);
        unsigned short completedStatus;
        __asm { fnstsw completedStatus }
        *status = completedStatus;
    }
    __except (CaptureException(GetExceptionInformation(), status)) {
        code = GetExceptionCode();
    }
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
    std::uint32_t random = 0x85a28173;
    for (unsigned short pc : precisions) for (unsigned short rc = 0; rc < 0x1000; rc += 0x400)
    {
        const unsigned short control = static_cast<unsigned short>((original & ~0xf00) | pc | rc | 0x3f);
        __asm { fldcw control }
        for (unsigned sample = 0; sample < 10000; ++sample) for (int shift = -3; shift <= 3; ++shift)
        {
            float a[12], b[12];
            for (unsigned i = 0; i < 12; ++i)
            {
                random ^= random << 13; random ^= random >> 17; random ^= random << 5;
                const auto bits = sample < 15 ? patterns[(sample + i) % 15] : random;
                std::memcpy(a + i, &bits, sizeof(bits));
            }
            std::memcpy(b, a, sizeof(a));
            unsigned short expectedStatus, actualStatus;
            ClearFlags();
            const unsigned mxcsr = _mm_getcsr();
            Reference(a + 4 + shift, a + 4);
            __asm { fnstsw expectedStatus }
            ClearFlags();
            Candidate(b + 4 + shift, b + 4);
            __asm { fnstsw actualStatus }
            if (std::memcmp(a, b, sizeof(a)) || expectedStatus != actualStatus || mxcsr != _mm_getcsr())
            {
                std::fprintf(stderr, "Mismatch sample=%u shift=%d pc=%x rc=%x status=%x/%x\n", sample, shift, pc, rc, expectedStatus, actualStatus);
                return 1;
            }
            ++cases;
        }
    }
    unsigned exceptionalCases = 0;
    for (unsigned short pc : precisions) for (unsigned short rc = 0; rc < 0x1000; rc += 0x400)
        for (unsigned mask : {1u, 2u}) for (unsigned component = 0; component < 3; ++component)
            for (int shift = -3; shift <= 3; ++shift)
            {
                float a[12], b[12];
                for (float& value : a) value = 1.0f;
                const std::uint32_t special = mask == 1 ? 0x7f812345u : 1u;
                std::memcpy(a + 4 + component, &special, sizeof(special));
                std::memcpy(b, a, sizeof(a));
                const unsigned short control = static_cast<unsigned short>(((original & ~0xf00) | pc | rc | 0x3f) & ~mask);
                unsigned short referenceStatus, candidateStatus;
                const auto referenceCode = RunUnmasked(Reference, a + 4 + shift, a + 4, control, &referenceStatus);
                const auto candidateCode = RunUnmasked(Candidate, b + 4 + shift, b + 4, control, &candidateStatus);
                if (referenceCode != candidateCode || referenceStatus != candidateStatus || std::memcmp(a, b, sizeof(a)))
                {
                    std::fprintf(stderr, "Unmasked mismatch component=%u mask=%u shift=%d codes=%x/%x status=%x/%x\n",
                                 component, mask, shift, referenceCode, candidateCode, referenceStatus, candidateStatus);
                    return 2;
                }
                ++exceptionalCases;
            }
    ClearFlags();
    __asm { fldcw original }
    std::printf("PASS: %u exact output/status comparisons, NaNs/subnormals/overlap, all PC/RC modes\n", cases);
    std::printf("PASS: %u unmasked invalid/denormal cases, matching exception code/status and partial output\n", exceptionalCases);
    return 0;
}
