#include "../../src/include/portable_rpc_quantization.h"
#include <cstdio>
#include <cstring>
#include <xmmintrin.h>

#if defined(TEST_FIRE_MEDIUM)
static const double Scale = 8192.0 / 32767.0;
#elif defined(TEST_FIRE_SMALL)
static const double Scale = 64.0 / 127.0;
#elif defined(TEST_EXPAND255)
static const double Scale = 1.0 / 255.0;
#elif defined(TEST_EXPAND_COMM)
static const double Scale = 2.0 / 51.0;
#else
static const double Scale = 1.0 / 127.0;
#endif

__declspec(noinline) void Reference(int value, float* output)
{
    __asm {
        mov ecx, output
        fild dword ptr value
        fmul qword ptr Scale
        fstp dword ptr [ecx]
    }
}

__declspec(noinline) void Candidate(int value, float* output)
{
    PortableRpcQuantization::ExpandInteger(value, Scale, output);
}

int main()
{
    unsigned short original;
    __asm { fnstcw original }
    const unsigned short precisions[] = {0, 0x200, 0x300};
    unsigned cases = 0, values = 0, statuses = 0, c1Changes = 0, mxcsrChanges = 0;
    for (unsigned short pc : precisions)
        for (unsigned short rc = 0; rc < 0x1000; rc += 0x400)
    {
        const unsigned short control = static_cast<unsigned short>((original & ~0xf00) | pc | rc | 0x3f);
        __asm { fldcw control }
#if defined(TEST_FIRE_MEDIUM)
        const int count = 65536;
#else
        const int count = 256;
#endif
        for (int byte = 0; byte < count; ++byte)
        {
#if defined(TEST_FIRE_MEDIUM)
            const int value = byte < 32768 ? byte : byte - 65536;
#elif defined(TEST_EXPAND255) || defined(TEST_EXPAND_COMM)
            const int value = byte;
#else
            const int value = byte < 128 ? byte : byte - 256;
#endif
            float a[] = {13.0f, 0.0f, 19.0f}, b[] = {13.0f, 0.0f, 19.0f};
            unsigned short expected, actual;
            const unsigned mxcsr = _mm_getcsr();
            __asm { fnclex }
            Reference(value, a + 1);
            __asm { fnstsw expected }
            __asm { fnclex }
            Candidate(value, b + 1);
            __asm { fnstsw actual }
            values += std::memcmp(a, b, sizeof(a)) != 0;
            // C1 is diagnostic: the source status consumers do not read it.
            // Retain exact checks of exception flags, stack and other status bits.
            statuses += ((expected ^ actual) & ~0x0200u) != 0;
            c1Changes += ((expected ^ actual) & 0x0200u) != 0;
            mxcsrChanges += mxcsr != _mm_getcsr();
            ++cases;
        }
    }
    __asm { fnclex }
    __asm { fldcw original }
    std::printf("%u comparisons: %u buffer differences, %u status differences outside C1, %u MXCSR changes; %u C1 diagnostics\n", cases, values, statuses, mxcsrChanges, c1Changes);
    return values || statuses || mxcsrChanges ? 1 : 0;
}
