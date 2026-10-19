#include "../../src/x86math/ScalarMath.h"
#include "candidates/legacy_scalar_math.h"
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <limits>
#include <initializer_list>
#include <xmmintrin.h>

static float FromBits(std::uint32_t bits)
{
    float value;
    std::memcpy(&value, &bits, sizeof(value));
    return value;
}

static std::uint32_t Bits(float value)
{
    std::uint32_t bits;
    std::memcpy(&bits, &value, sizeof(bits));
    return bits;
}

__declspec(noinline) static float NewSqrt(float x) { return X86ScalarMath::Sqrt(x); }
__declspec(noinline) static float NewInvSqrt(float x) { return X86ScalarMath::InvSqrt(x); }
__declspec(noinline) static float OldSqrt(float x) { return LegacyScalarMath::TableInvSqrt(x) * x; }
__declspec(noinline) static float OldInvSqrt(float x) { return LegacyScalarMath::TableInvSqrt(x); }

int main()
{
    unsigned short original;
    __asm { fnstcw original }
    const unsigned short measurement = static_cast<unsigned short>((original & ~0xf00) | 0x200 | 0x3f);
    const unsigned originalMxcsr = _mm_getcsr();
    std::uint32_t random = 0x59a31u;
    unsigned cases = 0, changedSqrt = 0, changedInv = 0, legacyNonfinite = 0;
    double maxSqrtResidual = 0, maxInvResidual = 0, maxLegacyRelativeChange = 0;
    double maxNormalRelativeChange = 0;
    std::uint32_t worstLegacyInput = 0;
    const std::uint32_t special[] = {0, 0x80000000u, 1, 0x007fffffu, 0x00800000u,
        0x3f800000u, 0x40800000u, 0x41800000u, 0x7f7fffffu, 0x7f800000u,
        0xff800000u, 0xbf800000u, 0x7fc12345u, 0x7f812345u};
    const unsigned short precision[] = {0, 0x200, 0x300};
    for (unsigned short pc : precision)
        for (unsigned short rc = 0; rc < 0x1000; rc += 0x400)
    {
        const unsigned short control = static_cast<unsigned short>((original & ~0xf00) | pc | rc | 0x3f);
        __asm { fldcw control }
        LegacyScalarMath::BuildInvSqrtTable();
        for (unsigned sample = 0; sample < 16384; ++sample)
        {
            random ^= random << 13; random ^= random >> 17; random ^= random << 5;
            std::uint32_t input = random & 0x7fffffffu;
            // Normal exponent boundaries plus special and random encodings.
            if (sample < 512) input = (sample / 2 << 23) | ((sample & 1) ? 0x7fffffu : 0);
            if (sample < sizeof(special) / sizeof(special[0])) input = special[sample];
            const float x = FromBits(input);
            __asm { fldcw control }
            const float root = NewSqrt(x), inverse = NewInvSqrt(x);
            unsigned short after;
            __asm { fnstcw after }
            if (after != control || ((_mm_getcsr() ^ originalMxcsr) & ~0x3fu)) return 1;
            const float oldRoot = OldSqrt(x), oldInverse = OldInvSqrt(x);
            __asm { fldcw measurement }
            ++cases;
            changedSqrt += Bits(root) != Bits(oldRoot);
            changedInv += Bits(inverse) != Bits(oldInverse);
            if (std::isfinite(x) && x > 0)
            {
                if (!std::isfinite(root) || root <= 0 || !std::isfinite(inverse) || inverse <= 0) return 2;
                const double rootResidual = std::fabs(static_cast<double>(root) * root / x - 1.0);
                const double invResidual = std::fabs(static_cast<double>(inverse) * inverse * x - 1.0);
                if (rootResidual > maxSqrtResidual) maxSqrtResidual = rootResidual;
                if (invResidual > maxInvResidual) maxInvResidual = invResidual;
                // Squared relative residual, not tolerance against the old approximation.
                if (rootResidual > 5e-7 || invResidual > 5e-7)
                {
                    std::fprintf(stderr, "Accuracy failure input=%08x pc=%x rc=%x root=%g inv=%g\n",
                        input, pc, rc, rootResidual, invResidual);
                    return 3;
                }
                if (!std::isfinite(oldRoot) || !std::isfinite(oldInverse)) ++legacyNonfinite;
                else
                {
                    const double change = std::fabs(static_cast<double>(oldInverse) / inverse - 1.0);
                    if (change > maxLegacyRelativeChange)
                    {
                        maxLegacyRelativeChange = change;
                        worstLegacyInput = input;
                    }
                    if (input >= 0x00800000u && change > maxNormalRelativeChange)
                        maxNormalRelativeChange = change;
                }
            }
            else if ((input & 0x7fffffffu) == 0)
            {
                if (Bits(root) != input || Bits(inverse) != (input | 0x7f800000u)) return 4;
            }
            else if (input == 0x7f800000u)
            {
                if (Bits(root) != input || Bits(inverse) != 0) return 5;
            }
            else if (!std::isnan(root) || !std::isnan(inverse)) return 6;
        }
    }
    __asm { fnclex }
    __asm { fldcw original }
    _mm_setcsr(originalMxcsr);
    std::printf("PASS: %u input pairs, all precision/rounding settings; control preserved\n", cases);
    std::printf("Maximum squared relative residual: sqrt %.9g, inverse %.9g (limit 5e-7)\n", maxSqrtResidual, maxInvResidual);
    std::printf("Legacy differences: sqrt %u, inverse %u; positive inputs with nonfinite legacy output %u\n",
        changedSqrt, changedInv, legacyNonfinite);
    std::printf("Largest inverse relative change where legacy results finite: %.9g at %08x; normal inputs %.9g\n",
        maxLegacyRelativeChange, worstLegacyInput, maxNormalRelativeChange);
    return 0;
}
