#include "candidates/spline_add_stored.h"
#include <windows.h>
#include <xmmintrin.h>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <initializer_list>

__declspec(noinline) void Reference(float* Dst, const float* A, const float* B)
{
    __asm {
        mov ecx, A
        mov edx, B
        mov eax, Dst
        fld dword ptr [ecx + 8]
        fadd dword ptr [edx + 8]
        fstp dword ptr [eax + 8]
        fld dword ptr [ecx + 4]
        fadd dword ptr [edx + 4]
        fstp dword ptr [eax + 4]
        fld dword ptr [ecx]
        fadd dword ptr [edx]
        fstp dword ptr [eax]
    }
}

__declspec(noinline) void Candidate(float* dst, const float* a, const float* b)
{
    CandidateAddStored(dst, a, b);
}

__declspec(noinline) void ClearFlags()
{
    __asm {
        fldz
        fstp st(0)
        fnclex
    }
}
using Kernel = void (*)(float*, const float*, const float*);
static int CaptureException(EXCEPTION_POINTERS* information, unsigned short* status)
{
    *status = static_cast<unsigned short>(information->ContextRecord->FloatSave.StatusWord);
    __asm { fnclex }
    return EXCEPTION_EXECUTE_HANDLER;
}
static unsigned RunUnmasked(Kernel kernel, float* dst, const float* a, const float* b,
                            unsigned short control, unsigned short* status)
{
    unsigned code = 0;
    ClearFlags();
    __asm { fldcw control }
    __try {
        kernel(dst, a, b);
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
    unsigned cases = 0, outputMismatches = 0, statusMismatches = 0, exceptional = 0, exceptionMismatches = 0;
    unsigned nonNanOutputMismatches = 0;
    std::uint32_t random = 0xb6512937;
    for (unsigned short pc : precisions) for (unsigned short rc = 0; rc < 0x1000; rc += 0x400)
    {
        const unsigned short control = static_cast<unsigned short>((original & ~0xf00) | pc | rc | 0x3f);
        __asm { fldcw control }
        for (unsigned sample = 0; sample < 10000; ++sample) for (int anchor : {6, 14}) for (int shift = -3; shift <= 3; ++shift)
        {
            float a[24], b[24];
            for (unsigned i = 0; i < 24; ++i)
            {
                random ^= random << 13; random ^= random >> 17; random ^= random << 5;
                const auto bits = sample < 225 ? patterns[((i >= 14 ? sample % 15 : sample / 15) + i) % 15] : random;
                std::memcpy(a + i, &bits, sizeof(bits));
            }
            std::memcpy(b, a, sizeof(a));
            unsigned short expectedStatus, actualStatus;
            ClearFlags();
            const unsigned mxcsr = _mm_getcsr();
            Reference(a + anchor + shift, a + 6, a + 14);
            __asm { fnstsw expectedStatus }
            ClearFlags();
            Candidate(b + anchor + shift, b + 6, b + 14);
            __asm { fnstsw actualStatus }
            if (std::memcmp(a, b, sizeof(a)) || expectedStatus != actualStatus || mxcsr != _mm_getcsr())
            {
                if (outputMismatches + statusMismatches < 3)
                    std::fprintf(stderr, "Mismatch sample=%u anchor=%d shift=%d pc=%x rc=%x status=%x/%x\n",
                                 sample, anchor, shift, pc, rc, expectedStatus, actualStatus);
                if (std::memcmp(a, b, sizeof(a))) {
                    ++outputMismatches;
                    for (unsigned i = 0; i < 24; ++i) {
                        std::uint32_t wordA, wordB;
                        std::memcpy(&wordA, a + i, sizeof(wordA));
                        std::memcpy(&wordB, b + i, sizeof(wordB));
                        const bool nanA = (wordA & 0x7f800000u) == 0x7f800000u && (wordA & 0x007fffffu);
                        const bool nanB = (wordB & 0x7f800000u) == 0x7f800000u && (wordB & 0x007fffffu);
                        if (wordA != wordB && !(nanA && nanB)) { ++nonNanOutputMismatches; break; }
                    }
                }
                if (expectedStatus != actualStatus || mxcsr != _mm_getcsr()) ++statusMismatches;
            }
            ++cases;
        }
    }
    for (unsigned short pc : precisions) for (unsigned short rc = 0; rc < 0x1000; rc += 0x400)
        for (unsigned mask : {1u, 2u, 8u}) for (unsigned side : {6u, 14u}) for (unsigned component = 0; component < 3; ++component)
            for (int anchor : {6, 14}) for (int shift = -3; shift <= 3; ++shift)
            {
                float a[24], b[24];
                for (float& value : a) value = 1.0f;
                const std::uint32_t special = mask == 1 ? 0x7f812345u : mask == 2 ? 1u : 0x7f7fffffu;
                std::memcpy(a + side + component, &special, sizeof(special));
                if (mask == 8) {
                    std::memcpy(a + 6 + component, &special, sizeof(special));
                    std::memcpy(a + 14 + component, &special, sizeof(special));
                }
                std::memcpy(b, a, sizeof(a));
                const unsigned short control = static_cast<unsigned short>(((original & ~0xf00) | pc | rc | 0x3f) & ~mask);
                unsigned short referenceStatus, candidateStatus;
                const auto expected = RunUnmasked(Reference, a + anchor + shift, a + 6, a + 14, control, &referenceStatus);
                const auto actual = RunUnmasked(Candidate, b + anchor + shift, b + 6, b + 14, control, &candidateStatus);
                if (expected != actual || referenceStatus != candidateStatus || std::memcmp(a, b, sizeof(a)))
                {
                    if (exceptionMismatches < 3)
                        std::fprintf(stderr, "Unmasked mismatch mask=%u side=%u component=%u anchor=%d shift=%d codes=%x/%x status=%x/%x\n",
                                     mask, side, component, anchor, shift, expected, actual, referenceStatus, candidateStatus);
                    ++exceptionMismatches;
                }
                ++exceptional;
            }
    ClearFlags();
    __asm { fldcw original }
    std::printf("%u masked cases: %u output mismatches, %u status mismatches\n", cases, outputMismatches, statusMismatches);
    std::printf("Output mismatches involving a non-NaN value: %u\n", nonNanOutputMismatches);
    std::printf("%u unmasked cases: %u exception/status/partial-output mismatches\n", exceptional, exceptionMismatches);
    return outputMismatches || statusMismatches || exceptionMismatches ? 1 : 0;
}
