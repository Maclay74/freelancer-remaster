#include "../../src/x86math/QuaternionMath.h"
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <xmmintrin.h>

struct Quaternion { float W, X, Y, Z; };
#include "candidates/legacy_quat_nlerp.h"

__declspec(noinline) static void Candidate(Quaternion& dst, const Quaternion& a,
                                          const Quaternion& b, const float* t)
{
    X86QuaternionMath::Nlerp(dst, a, b, t);
}

static float Special(std::uint32_t bits)
{
    float value;
    std::memcpy(&value, &bits, sizeof(value));
    return value;
}

static Quaternion RandomUnit(std::uint32_t& state)
{
    float values[4];
    double length = 0;
    for (auto& value : values)
    {
        state ^= state << 13; state ^= state >> 17; state ^= state << 5;
        value = static_cast<float>(static_cast<int>(state % 20001) - 10000);
        length += static_cast<double>(value) * value;
    }
    length = std::sqrt(length);
    return {static_cast<float>(values[0] / length), static_cast<float>(values[1] / length),
            static_cast<float>(values[2] / length), static_cast<float>(values[3] / length)};
}

int main()
{
    unsigned short original;
    __asm { fnstcw original }
    const unsigned short measure = static_cast<unsigned short>((original & ~0xf00) | 0x200 | 0x3f);
    const unsigned originalMxcsr = _mm_getcsr();
    std::uint32_t random = 0x6dc341u;
    unsigned cases = 0, differing = 0, flagDifferences = 0;
    double maximumError = 0, maximumLengthError = 0;
    const unsigned short precision[] = {0, 0x200, 0x300};
    const float Quaternion::* lanes[] = {&Quaternion::W, &Quaternion::X, &Quaternion::Y, &Quaternion::Z};
    const float fractions[] = {0, 0.125f, 0.5f, 0.875f, 1, -0.5f, 1.5f, Special(0x7fc12345)};
    for (auto pc : precision) for (unsigned short rc = 0; rc < 0x1000; rc += 0x400)
    {
        const unsigned short control = static_cast<unsigned short>((original & ~0xf00) | pc | rc | 0x3f);
        for (unsigned sample = 0; sample < 256; ++sample)
        {
            __asm { fldcw measure }
            Quaternion a = RandomUnit(random), b = RandomUnit(random);
            switch (sample)
            {
            case 0: a = b = {1, 0, 0, 0}; break;
            case 1: a = {1, 0, 0, 0}; b = {-1, 0, 0, 0}; break;
            case 2: a = {1, 0, 0, 0}; b = {0, 1, 0, 0}; break;
            case 3: a = {0.5f, 0.5f, 0.5f, 0.5f}; b = {0.5f, -0.5f, 0.5f, -0.5f}; break;
            case 4: a = b = {0, 0, 0, 0}; break;
            case 5: a = b = {-0.0f, 0, -0.0f, 0}; break;
            case 6: a = b = {1e-40f, -1e-40f, 1e-40f, -1e-40f}; break;
            case 7: a = b = {1e38f, -1e38f, 1e38f, -1e38f}; break;
            case 8: a.W = Special(0x7fc12345); break;
            case 9: b.X = Special(0x7f812345); break;
            case 10: a.Y = Special(0x7f800000); break;
            case 11: b = {-a.X, a.W, -a.Z, a.Y}; break;
            case 12:
                b = {-a.X, a.W, -a.Z, a.Y};
                b.W = std::nextafter(b.W, 1.0f);
                break;
            default: break;
            }
            for (float fraction : fractions) for (unsigned alias = 0; alias < 7; ++alias)
            {
                Quaternion expected[] = {{0.25f, -0.25f, 0.75f, -0.75f}, a, b};
                Quaternion actual[3];
                std::memcpy(actual, expected, sizeof(actual));
                const unsigned output = alias < 3 ? alias : 0;
                const float* oldT = alias < 3 ? &fraction : &(expected[output].*lanes[alias - 3]);
                const float* newT = alias < 3 ? &fraction : &(actual[output].*lanes[alias - 3]);
                unsigned short oldStatus, newStatus, after;
                __asm { fldcw control }
                __asm { fnclex }
                LegacyNlerp(expected[output], expected[1], expected[2], oldT);
                __asm { fnstsw oldStatus }
                __asm { fnclex }
                Candidate(actual[output], actual[1], actual[2], newT);
                __asm { fnstsw newStatus }
                __asm { fnstcw after }
                if (after != control || ((_mm_getcsr() ^ originalMxcsr) & ~0x3fu) ||
                    ((oldStatus ^ newStatus) & 0x3800)) return 1;
                __asm { fldcw measure }
                ++cases;
                flagDifferences += ((oldStatus ^ newStatus) & 0x3f) != 0;
                differing += std::memcmp(expected, actual, sizeof(actual)) != 0;
                double length = 0;
                bool finite = true;
                for (unsigned index = 0; index < 4; ++index)
                {
                    const float oldValue = expected[output].*lanes[index], newValue = actual[output].*lanes[index];
                    if (std::isnan(oldValue) && std::isnan(newValue)) { finite = false; continue; }
                    if (std::isinf(oldValue) && oldValue == newValue) { finite = false; continue; }
                    const double error = std::fabs(static_cast<double>(oldValue) - newValue);
                    if (!std::isfinite(error) || error > 2e-6)
                    {
                        std::fprintf(stderr, "Mismatch sample=%u alias=%u t=%g pc=%x rc=%x lane=%u old=%g new=%g error=%g\n",
                            sample, alias, fraction, pc, rc, index, oldValue, newValue, error);
                        return 2;
                    }
                    if (error > maximumError) maximumError = error;
                    length += static_cast<double>(newValue) * newValue;
                }
                if (finite)
                {
                    const double error = std::fabs(length - 1.0);
                    if (error > maximumLengthError) maximumLengthError = error;
                    if (error > 2e-6) return 3;
                }
                for (unsigned index = 0; index < 3; ++index)
                    if (index != output && std::memcmp(expected + index, actual + index, sizeof(Quaternion))) return 4;
            }
        }
    }
    __asm { fnclex }
    __asm { fldcw original }
    _mm_setcsr(originalMxcsr);
    std::printf("PASS: %u nlerp cases; max component error %.9g; max squared-length error %.9g (limits 2e-6)\n",
        cases, maximumError, maximumLengthError);
    std::printf("Diagnostics: %u bitwise-different buffers; %u exception-flag differences; controls/stack preserved\n",
        differing, flagDifferences);
    return 0;
}
