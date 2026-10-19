#include "../../src/include/portable_rpc_server_quantization.h"
#include "candidates/legacy_rpc_server_quantization.h"
#include <climits>
#include <cstdio>
#include <xmmintrin.h>

#if defined(TEST_HULL_EXPAND)
__declspec(noinline) static int Candidate(unsigned char encoded, int maximum)
{
    return PortableRpcServerQuantization::ExpandHull(encoded, maximum);
}
#else
__declspec(noinline) static unsigned char Candidate(int points, int maximum, unsigned short* callback)
{
    const float rounded = PortableRpcServerQuantization::PrepareHull(points);
    unsigned short status;
    __asm { fnstsw status }
    *callback = status;
    return PortableRpcServerQuantization::QuantizeHull(rounded, maximum);
}
#endif

int main()
{
    const int values[] = {0, 1, -1, 2, 3, 5, 17, 51, 85, 100, 255, 256, 257,
        510, 511, 512, 16777215, 16777216, 16777217, INT_MAX, INT_MIN};
    const unsigned count = sizeof(values) / sizeof(values[0]);
    const unsigned short precision[] = {0, 0x200, 0x300};
    unsigned short original;
    __asm { fnstcw original }
    const unsigned originalMxcsr = _mm_getcsr();
    unsigned cases = 0;
    std::uint32_t random = 0x518c24u;
    for (auto pc : precision) for (unsigned short rc = 0; rc < 0x1000; rc += 0x400)
    {
        const unsigned short control = static_cast<unsigned short>((original & ~0xf00) | pc | rc | 0x3f);
        __asm { fldcw control }
#if defined(TEST_HULL_EXPAND)
        for (unsigned encoded = 0; encoded < 256; ++encoded)
            for (unsigned sample = 0; sample < count + 16; ++sample)
#else
        for (unsigned sample = 0; sample < 1024; ++sample)
#endif
        {
            random ^= random << 13; random ^= random >> 17; random ^= random << 5;
            int points, maximum;
            std::memcpy(&points, &random, 4);
            maximum = points;
#if defined(TEST_HULL_EXPAND)
            if (sample < count) maximum = values[sample];
#else
            random ^= random << 13; random ^= random >> 17; random ^= random << 5;
            std::memcpy(&maximum, &random, 4);
            if (sample < count * count) { points = values[sample % count]; maximum = values[sample / count]; }
#endif
            unsigned short expectedStatus, actualStatus, after, expectedCallback = 0, actualCallback = 0;
            __asm { fnclex }
#if defined(TEST_HULL_EXPAND)
            const auto expected = LegacyServerQuantization::RpcServerExpandHull(static_cast<unsigned char>(encoded), maximum);
#else
            const auto expected = LegacyServerQuantization::RpcServerQuantizeHull(points, maximum, &expectedCallback);
#endif
            __asm { fnstsw expectedStatus }
            __asm { fnclex }
#if defined(TEST_HULL_EXPAND)
            const auto actual = Candidate(static_cast<unsigned char>(encoded), maximum);
#else
            const auto actual = Candidate(points, maximum, &actualCallback);
#endif
            __asm { fnstsw actualStatus }
            __asm { fnstcw after }
            if (actual != expected || ((expectedStatus ^ actualStatus) & 0x383f) ||
                ((expectedCallback ^ actualCallback) & 0x383f) || after != control || originalMxcsr != _mm_getcsr())
            {
                std::fprintf(stderr, "Mismatch sample=%u points=%d max=%d pc=%x rc=%x value=%d/%d status=%x/%x callback=%x/%x\n",
                    sample, points, maximum, pc, rc, static_cast<int>(expected), static_cast<int>(actual), expectedStatus,
                    actualStatus, expectedCallback, actualCallback);
                return 1;
            }
            ++cases;
        }
    }
    __asm { fnclex }
    __asm { fldcw original }
    std::printf("PASS: %u exact hull result/exception/stack/callback-state comparisons; controls/MXCSR unchanged\n", cases);
    return 0;
}
