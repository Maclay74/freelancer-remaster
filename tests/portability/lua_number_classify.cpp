#include "../../src/include/portable_lua_number.h"
using real = double;
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <xmmintrin.h>

__declspec(noinline) int Reference(const double* Number, unsigned int* converted, int* negative)
{
  static const real Zero = 0;
  static const real Maximum = (real)65535;
  real Absolute;
  int bInBounds;
  unsigned short ComparisonStatus;
  __asm {
    mov eax, Number
    fld qword ptr [eax]
    fcomp qword ptr Zero
    fld qword ptr [eax]
    fnstsw ax
    test ah, 1
    jz NumberAbsoluteReady
    fchs
  NumberAbsoluteReady:
    fld qword ptr Zero
    fcomp st(1)
    fnstsw ax
    test ah, 41h
    jz NumberBoundsRejected
    fcom qword ptr Maximum
    fnstsw ax
    test ah, 41h
    jz NumberBoundsRejected
    /* Exact decoded-double/FCHS transport, retaining the original ST0. */
    fst qword ptr Absolute
    mov bInBounds, 1
    jmp NumberBoundsFinished
  NumberBoundsRejected:
    fstp st(0)
    mov bInBounds, 0
  NumberBoundsFinished:
  }
  if (bInBounds) {
    /* Modern signed-64 truncation consumes a duplicate; the native listing
       must retain the owned absolute below it. This is not a CRT clone. */
    const __int64 Integral = (__int64)Absolute;
    const int Converted = (int)(unsigned int)Integral;
    __asm {
      fild dword ptr Converted
      fcomp st(1)
      fnstsw ComparisonStatus
      fstp st(0)
    }
    /* AH40/JZ rejects unequal only; unordered has C3 and is accepted. */
    if (ComparisonStatus & 0x4000) {
      __asm {
        mov eax, Number
        fld qword ptr [eax]
        fcomp qword ptr Zero
        fnstsw ComparisonStatus
      }
      /* AH01/JNZ selects negative or unordered opcode9, else opcode7. */
      *converted = (unsigned int)Converted;
      *negative = (ComparisonStatus & 0x0100) != 0;
      return 1;
    }
  }
  return 0;
}

__declspec(noinline) int Candidate(const double* input, unsigned int* converted, int* negative)
{
    return PortableLuaClassifyNumber(input, 65535, converted, negative);
}

int main()
{
    const std::uint64_t patterns[] = {0, UINT64_C(0x8000000000000000), 1,
        UINT64_C(0x8000000000000001), UINT64_C(0x000fffffffffffff),
        UINT64_C(0x0010000000000000), UINT64_C(0x3fefffffffffffff),
        UINT64_C(0x3ff0000000000000), UINT64_C(0x3ff0000000000001),
        UINT64_C(0x3ff8000000000000), UINT64_C(0xbff8000000000000),
        UINT64_C(0x43dfffffffffffff), UINT64_C(0x43e0000000000000),
        UINT64_C(0xc3e0000000000000), UINT64_C(0xc3e0000000000001),
        UINT64_C(0x7ff0000000000000), UINT64_C(0xfff0000000000000),
        UINT64_C(0x7ff8123456789abc), UINT64_C(0x7ff0123456789abc), UINT64_C(0xfff8123456789abc),
        UINT64_C(0x40efffe000000000), UINT64_C(0xc0efffe000000000),
        UINT64_C(0x40efffdfffffffff), UINT64_C(0x40efffe000000001)};
    unsigned short original;
    __asm { fnstcw original }
    const unsigned short precisions[] = {0, 0x200, 0x300};
    std::uint64_t random = UINT64_C(0x48127ac319752);
    unsigned cases = 0, outputs = 0, flags = 0, controls = 0, mxcsrChanges = 0;
    for (unsigned short pc : precisions)
        for (unsigned short rc = 0; rc < 0x1000; rc += 0x400)
    {
        const unsigned short control = static_cast<unsigned short>((original & ~0xf00) | pc | rc | 0x3f);
        __asm { fldcw control }
        for (unsigned sample = 0; sample < 4096; ++sample)
        {
            random ^= random << 13; random ^= random >> 7; random ^= random << 17;
            const std::uint64_t bits = sample < sizeof(patterns) / sizeof(patterns[0]) ? patterns[sample] : random;
            double input;
            std::memcpy(&input, &bits, sizeof(input));
            unsigned short expectedStatus, actualStatus, after;
            const unsigned mxcsr = _mm_getcsr();
            __asm { fnclex }
            unsigned int expectedArgument = 0xdeadbeef, actualArgument = expectedArgument;
            int expectedNegative = 17, actualNegative = expectedNegative;
            const auto expected = Reference(&input, &expectedArgument, &expectedNegative);
            __asm { fnstsw expectedStatus }
            __asm { fnclex }
            const auto actual = Candidate(&input, &actualArgument, &actualNegative);
            __asm { fnstsw actualStatus }
            __asm { fnstcw after }
            outputs += expected != actual || (expected && (expectedArgument != actualArgument || expectedNegative != actualNegative));
            flags += ((expectedStatus ^ actualStatus) & 0x383f) != 0;
            controls += after != control;
            mxcsrChanges += mxcsr != _mm_getcsr();
            ++cases;
        }
    }
    __asm { fnclex }
    __asm { fldcw original }
    std::printf("%u comparisons: %u result differences, %u exception differences, %u control changes, %u MXCSR changes\n", cases, outputs, flags, controls, mxcsrChanges);
    return outputs || flags || controls || mxcsrChanges ? 1 : 0;
}
