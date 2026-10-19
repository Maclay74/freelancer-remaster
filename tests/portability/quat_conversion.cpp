#include "../../src/x86math/QuaternionMath.h"
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <algorithm>
#include <xmmintrin.h>

struct FQuaternion { float W, X, Y, Z; };
struct FMatrix { float D[3][3]; };
#include "candidates/legacy_quat_conversion.h"

__declspec(noinline) static void ToMatrix(FMatrix& dst, const FQuaternion& q)
{
    X86QuaternionMath::ToMatrix(dst, q);
}
__declspec(noinline) static void FromMatrix(FQuaternion& dst, const FMatrix& matrix)
{
    X86QuaternionMath::FromMatrix(dst, matrix);
}
static float Random(std::uint32_t& state)
{
    state ^= state << 13; state ^= state >> 17; state ^= state << 5;
    return static_cast<float>(static_cast<int>(state % 20001) - 10000) / 10000.0f;
}
template<class Value>
static double Difference(const Value& a, const Value& b)
{
    float left[sizeof(Value) / sizeof(float)], right[sizeof(Value) / sizeof(float)];
    std::memcpy(left, &a, sizeof(a)); std::memcpy(right, &b, sizeof(b));
    double maximum = 0;
    for (unsigned i = 0; i < sizeof(left) / sizeof(left[0]); ++i)
    {
        if (std::isnan(left[i]) && std::isnan(right[i])) continue;
        const double error = std::fabs(static_cast<double>(left[i]) - right[i]);
        if (!std::isfinite(error)) return INFINITY;
        maximum = std::max(maximum, error);
    }
    return maximum;
}

int main()
{
    unsigned short original;
    __asm { fnstcw original }
    const unsigned short measurement = static_cast<unsigned short>((original & ~0xf00) | 0x200 | 0x3f);
    const unsigned originalMxcsr = _mm_getcsr();
    std::uint32_t random = 0x15bd4u;
    unsigned cases = 0;
    double maxTo = 0, maxFrom = 0, maxRoundTrip = 0;
    const unsigned short precision[] = {0, 0x200, 0x300};
    const double scales[] = {1.0, 2.0, 1e-12, 1e12};
    for (auto pc : precision) for (unsigned short rc = 0; rc < 0x1000; rc += 0x400)
        for (unsigned sample = 0; sample < 512; ++sample)
    {
        __asm { fldcw measurement }
        FQuaternion q = {Random(random), Random(random), Random(random), Random(random)};
        if (sample == 0) q = {1, 0, 0, 0};
        if (sample == 1) q = {0, 1, 0, 0};
        if (sample == 2) q = {0, 0, 1, 0};
        if (sample == 3) q = {0, 0, 0, 1};
        if (sample == 4) q = {0.5f, 0.5f, 0.5f, 0.5f};
        if (sample == 5) q = {-0.5f, -0.5f, -0.5f, -0.5f};
        const double scale = scales[sample % 4];
        q.W = static_cast<float>(q.W * scale); q.X = static_cast<float>(q.X * scale);
        q.Y = static_cast<float>(q.Y * scale); q.Z = static_cast<float>(q.Z * scale);
        FMatrix oldMatrix{}, newMatrix{}, roundTrip{};
        FQuaternion oldQ{}, newQ{};
        const unsigned short control = static_cast<unsigned short>((original & ~0xf00) | pc | rc | 0x3f);
        __asm { fldcw control }
        __asm { fnclex }
        LegacyQuaternionMath::QuaternionToMatrix(oldMatrix, q);
        ToMatrix(newMatrix, q);
        LegacyQuaternionMath::MatrixToQuaternion(oldQ, oldMatrix);
        FromMatrix(newQ, oldMatrix);
        ToMatrix(roundTrip, newQ);
        unsigned short after, status;
        __asm { fnstcw after }
        __asm { fnstsw status }
        if (after != control || (status & 0x3800) || ((_mm_getcsr() ^ originalMxcsr) & ~0x3fu)) return 1;
        __asm { fldcw measurement }
        const double toError = Difference(oldMatrix, newMatrix);
        const double fromError = Difference(oldQ, newQ);
        const double roundTripError = Difference(oldMatrix, roundTrip);
        if (toError > 2e-6 || fromError > 2e-6 || roundTripError > 2e-6)
        {
            std::fprintf(stderr, "Mismatch sample=%u pc=%x rc=%x to=%g from=%g roundtrip=%g\n",
                sample, pc, rc, toError, fromError, roundTripError);
            return 2;
        }
        maxTo = std::max(maxTo, toError); maxFrom = std::max(maxFrom, fromError);
        maxRoundTrip = std::max(maxRoundTrip, roundTripError);
        ++cases;
    }
    __asm { fldcw measurement }
    const FQuaternion zero{};
    FMatrix zeroMatrix;
    ToMatrix(zeroMatrix, zero);
    for (const auto& row : zeroMatrix.D) for (float value : row) if (!std::isnan(value)) return 3;
    __asm { fnclex }
    __asm { fldcw original }
    _mm_setcsr(originalMxcsr);
    std::printf("PASS: %u quaternion conversion pairs; absolute errors to %.9g, from %.9g, roundtrip %.9g (limits 2e-6); controls/stack preserved\n",
        cases, maxTo, maxFrom, maxRoundTrip);
    return 0;
}
