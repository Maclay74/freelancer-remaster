#include "../../src/include/portable_rpc_server_quantization.h"
#include "candidates/legacy_rpc_server_quantization.h"
#include <cstdio>
#include <xmmintrin.h>

__declspec(noinline) static unsigned char Candidate(const float* input)
{
    return static_cast<unsigned char>(PortableRpcServerQuantization::QuantizeComm(input));
}

int main()
{
    const std::uint32_t patterns[] = {0, 0x80000000u, 0x3f800000u, 0xbf800000u,
        0x7f800000u, 0xff800000u, 0x7fc12345u, 0xffc54321u, 0x7f812345u,
        0xff854321u, 1, 0x80000001u, 0x007fffffu, 0x00800000u, 0x7f7fffffu};
    const unsigned short precision[] = {0, 0x200, 0x300};
    unsigned short original;
    __asm { fnstcw original }
    const unsigned short measurement = static_cast<unsigned short>((original & ~0xf00) | 0x200 | 0x3f);
    const unsigned originalMxcsr = _mm_getcsr();
    unsigned cases = 0;
    std::uint32_t random = 0x198bc3u;
    for (auto pc : precision) for (unsigned short rc = 0; rc < 0x1000; rc += 0x400)
        for (unsigned sample = 0; sample < 4096; ++sample)
    {
        __asm { fldcw measurement }
        random ^= random << 13; random ^= random >> 17; random ^= random << 5;
        std::uint32_t bits = random;
        if (sample < 768)
        {
            const float boundary = static_cast<float>((sample / 3) / (static_cast<double>(0.1f) * 255.0));
            std::memcpy(&bits, &boundary, 4);
            bits += sample % 3; // boundary and its two following encodings
        }
        if (sample < sizeof(patterns) / sizeof(patterns[0])) bits = patterns[sample];
        float input;
        std::memcpy(&input, &bits, 4);
        const unsigned short control = static_cast<unsigned short>((original & ~0xf00) | pc | rc | 0x3f);
        unsigned short expectedStatus, actualStatus, after;
        __asm { fldcw control }
        __asm { fnclex }
        const auto expected = LegacyServerQuantization::RpcServerSendCommQuantize(&input);
        __asm { fnstsw expectedStatus }
        __asm { fnclex }
        const auto actual = Candidate(&input);
        __asm { fnstsw actualStatus }
        __asm { fnstcw after }
        if (actual != expected || ((expectedStatus ^ actualStatus) & 0x383f) || after != control || originalMxcsr != _mm_getcsr())
        {
            std::fprintf(stderr, "Mismatch sample=%u bits=%08x pc=%x rc=%x value=%u/%u status=%x/%x\n",
                sample, bits, pc, rc, expected, actual, expectedStatus, actualStatus);
            return 1;
        }
        ++cases;
    }
    __asm { fnclex }
    __asm { fldcw original }
    std::printf("PASS: %u exact communication byte/exception/stack comparisons; controls/MXCSR unchanged\n", cases);
    return 0;
}
