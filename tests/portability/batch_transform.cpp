#include "../../src/x86math/BatchTransform.h"
#include <cmath>
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <algorithm>
#include <xmmintrin.h>

struct FVector { float X, Y, Z; };
struct FMatrix { float D[3][3]; };
#include "candidates/legacy_batch_transform.h"

__declspec(noinline) static void Candidate(FVector* dst, const FMatrix& matrix,
                                          const FVector* src, int count, const FVector* offset)
{
    X86BatchMath::TransformList(dst, matrix, src, count, offset);
}

static float RandomValue(std::uint32_t& state)
{
    state ^= state << 13; state ^= state >> 17; state ^= state << 5;
    return static_cast<float>(static_cast<int>(state % 20001) - 10000) / 127.0f;
}

// Independent row/column formulation; records a cancellation-aware error scale.
static void Model(FVector* dst, const FMatrix& matrix, const FVector* src, int count,
                  const FVector* translation, double scales[8][3])
{
    const FMatrix coefficients = matrix;
    const FVector offset = translation ? *translation : FVector{};
    for (int index = 0; index < count; ++index)
    {
        const double input[] = {src[index].X, src[index].Y, src[index].Z};
        const double additions[] = {offset.X, offset.Y, offset.Z};
        float output[3];
        for (int row = 0; row < 3; ++row)
        {
            long double sum = additions[row];
            double magnitude = std::fabs(additions[row]);
            for (int col = 0; col < 3; ++col)
            {
                sum += static_cast<long double>(coefficients.D[row][col]) * input[col];
                magnitude += std::fabs(static_cast<double>(coefficients.D[row][col]) * input[col]);
            }
            output[row] = static_cast<float>(sum);
            scales[index][row] = std::max(1.0, magnitude);
        }
        dst[index] = {output[0], output[1], output[2]};
    }
}

int main()
{
    unsigned short original;
    __asm { fnstcw original }
    const unsigned short measurement = static_cast<unsigned short>((original & ~0xf00) | 0x200 | 0x3f);
    const unsigned originalMxcsr = _mm_getcsr();
    std::uint32_t random = 0x3a7182u;
    unsigned cases = 0, legacyComparisons = 0, legacyAliasDifferences = 0;
    double maxModelError = 0, maxLegacyError = 0;
    const unsigned short precision[] = {0, 0x200, 0x300};
    const int counts[] = {-1, 0, 1, 8};
    const float coefficients[] = {0, 1, -2};
    for (auto pc : precision) for (unsigned short rc = 0; rc < 0x1000; rc += 0x400)
    {
        const unsigned short control = static_cast<unsigned short>((original & ~0xf00) | pc | rc | 0x3f);
        for (unsigned sample = 0; sample < 128; ++sample)
        {
            __asm { fldcw measurement }
            FMatrix matrix;
            for (int row = 0; row < 3; ++row) for (int col = 0; col < 3; ++col)
                matrix.D[row][col] = RandomValue(random) / 31.0f;
            if (sample < 27)
                for (int row = 0; row < 3; ++row)
                {
                    const unsigned id = (sample + row * 7) % 27;
                    matrix.D[row][0] = coefficients[id / 9];
                    matrix.D[row][1] = coefficients[id / 3 % 3];
                    matrix.D[row][2] = coefficients[id % 3];
                }
            if (sample == 27) matrix = {{{1, 0, 0}, {0, 1, 0}, {0, 0, 1}}};
            if (sample == 28) matrix = {};
            if (sample == 29) matrix = {{{0, -1, 0}, {1, 0, 0}, {0, 0, 1}}};
            for (unsigned offsetMode = 0; offsetMode < 4; ++offsetMode)
                for (int count : counts) for (unsigned alias = 0; alias < 2; ++alias)
            {
                FVector initial[24], expected[24], actual[24], oldBank[24];
                for (auto& value : initial)
                    value = {RandomValue(random), RandomValue(random), RandomValue(random)};
                std::memcpy(expected, initial, sizeof(initial));
                std::memcpy(actual, initial, sizeof(initial));
                std::memcpy(oldBank, initial, sizeof(initial));
                const unsigned output = alias == 0 ? 14 : 4;
                const FVector offset = offsetMode == 1 ? FVector{} : FVector{10.5f, -20.25f, 30.75f};
                const FVector* expectedOffset = offsetMode == 0 ? nullptr : offsetMode == 3 ? expected + output : &offset;
                const FVector* actualOffset = offsetMode == 0 ? nullptr : offsetMode == 3 ? actual + output : &offset;
                const FVector savedOffset = expectedOffset ? *expectedOffset : FVector{};
                double scales[8][3]{};
                Model(expected + output, matrix, expected + 4, count, expectedOffset, scales);
                __asm { fldcw control }
                __asm { fnclex }
                Candidate(actual + output, matrix, actual + 4, count, actualOffset);
                unsigned short after, status;
                __asm { fnstcw after }
                __asm { fnstsw status }
                if (after != control || (status & 0x3800) || ((_mm_getcsr() ^ originalMxcsr) & ~0x3fu)) return 1;
                FVector oldOutput[8]{};
                if (count > 0 && alias < 2)
                {
                    LegacyBatchMath::TransformListFast(oldOutput, matrix, initial + 4, count,
                        offsetMode == 0 ? nullptr : &savedOffset);
                    if (alias == 1)
                        LegacyBatchMath::TransformListFast(oldBank + output, matrix, oldBank + 4, count,
                            offsetMode == 0 ? nullptr : &savedOffset);
                }
                __asm { fldcw measurement }
                bool aliasDifferent = false;
                for (int index = 0; index < count; ++index)
                {
                    const float model[] = {expected[output + index].X, expected[output + index].Y, expected[output + index].Z};
                    const float result[] = {actual[output + index].X, actual[output + index].Y, actual[output + index].Z};
                    const float old[] = {oldOutput[index].X, oldOutput[index].Y, oldOutput[index].Z};
                    const float oldAlias[] = {oldBank[output + index].X, oldBank[output + index].Y, oldBank[output + index].Z};
                    for (int row = 0; row < 3; ++row)
                    {
                        const double error = std::fabs(static_cast<double>(result[row]) - model[row]) / scales[index][row];
                        maxModelError = std::max(maxModelError, error);
                        double legacyError = 0;
                        if (alias < 2)
                        {
                            legacyError = std::fabs(static_cast<double>(result[row]) - old[row]) / scales[index][row];
                            maxLegacyError = std::max(maxLegacyError, legacyError);
                            aliasDifferent |= alias == 1 && std::fabs(static_cast<double>(result[row]) - oldAlias[row]) / scales[index][row] > 2e-6;
                        }
                        if (!std::isfinite(error) || error > 2e-6 || legacyError > 2e-6)
                        {
                            std::fprintf(stderr, "Mismatch sample=%u alias=%u offset=%u count=%d pc=%x rc=%x model=%g legacy=%g\n",
                                sample, alias, offsetMode, count, pc, rc, error, legacyError);
                            return 2;
                        }
                    }
                }
                for (unsigned index = 0; index < 24; ++index)
                    if ((count <= 0 || index < output || index >= output + count) &&
                        std::memcmp(initial + index, actual + index, sizeof(FVector))) return 3;
                ++cases;
                legacyComparisons += count > 0 && alias < 2;
                legacyAliasDifferences += aliasDifferent;
            }
        }
    }
    __asm { fldcw measurement }
    const FMatrix identity = {{{1, 0, 0}, {0, 1, 0}, {0, 0, 1}}};
    X86BatchMath::TransformList<FVector>(nullptr, identity, nullptr, 0);
    X86BatchMath::TransformList<FVector>(nullptr, identity, nullptr, -1);
    const FVector special = {INFINITY, 3, 4};
    FVector specialResult;
    Candidate(&specialResult, identity, &special, 1, nullptr);
    if (!std::isinf(specialResult.X) || !std::isnan(specialResult.Y) || !std::isnan(specialResult.Z)) return 4;
    __asm { fnclex }
    __asm { fldcw original }
    _mm_setcsr(originalMxcsr);
    std::printf("PASS: %u batch cases; %u legacy out-of-place comparisons\n", cases, legacyComparisons);
    std::printf("Maximum scaled errors: model %.9g, legacy %.9g (limit 2e-6)\n", maxModelError, maxLegacyError);
    std::printf("Legacy in-place alias discrepancies: %u batches; new loop snapshots each source vector\n", legacyAliasDifferences);
    return 0;
}
