#include "../../src/x86math/TransformMath.h"
#include <cmath>
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <algorithm>
#include <xmmintrin.h>

struct FVector { float X, Y, Z; };
struct FMatrix { float D[3][3]; };
struct FTransform { FMatrix Rotation; FVector Translation; };
#include "candidates/legacy_x86_core.h"

struct Outputs { FVector vector, transposed, affine; FMatrix product; FTransform composition; };

static float Random(std::uint32_t& state)
{
    state ^= state << 13; state ^= state >> 17; state ^= state << 5;
    return static_cast<float>(static_cast<int>(state % 20001) - 10000) / 1024.0f;
}

__declspec(noinline) static void Candidate(Outputs& out, const FTransform& a, const FTransform& b,
                                          const FVector& vector)
{
    X86MatrixMath::TransformVector(out.vector, a.Rotation, vector);
    X86MatrixMath::TransformTranspose(out.transposed, a.Rotation, vector);
    X86TransformMath::Apply(out.affine, a, vector);
    X86MatrixMath::Multiply(out.product, a.Rotation, b.Rotation);
    X86TransformMath::Compose(out.composition, a, b);
}

static double Scale(const FMatrix& matrix, const FVector& vector, int row, bool transpose = false)
{
    const double v[] = {vector.X, vector.Y, vector.Z};
    double magnitude = 0;
    for (int col = 0; col < 3; ++col)
        magnitude += std::fabs(static_cast<double>(transpose ? matrix.D[col][row] : matrix.D[row][col]) * v[col]);
    return magnitude;
}

int main()
{
    static_assert(sizeof(Outputs) == 30 * sizeof(float), "Packed float outputs");
    unsigned short original;
    __asm { fnstcw original }
    const unsigned short measurement = static_cast<unsigned short>((original & ~0xf00) | 0x200 | 0x3f);
    const unsigned originalMxcsr = _mm_getcsr();
    std::uint32_t random = 0x4bc21u;
    unsigned cases = 0;
    double maximum = 0;
    const unsigned short precision[] = {0, 0x200, 0x300};
    for (auto pc : precision) for (unsigned short rc = 0; rc < 0x1000; rc += 0x400)
        for (unsigned sample = 0; sample < 512; ++sample)
    {
        __asm { fldcw measurement }
        FTransform a, b;
        for (int row = 0; row < 3; ++row) for (int col = 0; col < 3; ++col)
        {
            a.Rotation.D[row][col] = Random(random);
            b.Rotation.D[row][col] = Random(random);
        }
        if (sample == 0) a.Rotation = b.Rotation = {{{1, 0, 0}, {0, 1, 0}, {0, 0, 1}}};
        if (sample == 1) a.Rotation = {};
        if (sample == 2) a.Rotation = {{{0, -1, 0}, {1, 0, 0}, {0, 0, 1}}};
        if (sample == 3) a.Rotation = {{{-2, 0, 0}, {0, 3, 0}, {0, 0, 0.5f}}};
        a.Translation = {Random(random), Random(random), Random(random)};
        b.Translation = {Random(random), Random(random), Random(random)};
        const FVector input = {Random(random), Random(random), Random(random)};
        double scales[30]{};
        const double translation[] = {a.Translation.X, a.Translation.Y, a.Translation.Z};
        for (int row = 0; row < 3; ++row)
        {
            scales[row] = Scale(a.Rotation, input, row);
            scales[3 + row] = Scale(a.Rotation, input, row, true);
            scales[6 + row] = scales[row] + std::fabs(translation[row]);
            for (int col = 0; col < 3; ++col)
            {
                const FVector column = {b.Rotation.D[0][col], b.Rotation.D[1][col], b.Rotation.D[2][col]};
                scales[9 + row * 3 + col] = scales[18 + row * 3 + col] = Scale(a.Rotation, column, row);
            }
            scales[27 + row] = Scale(a.Rotation, b.Translation, row) + std::fabs(translation[row]);
        }
        Outputs expected{}, actual{};
        const unsigned short control = static_cast<unsigned short>((original & ~0xf00) | pc | rc | 0x3f);
        __asm { fldcw control }
        __asm { fnclex }
        LegacyCoreMath::MulVecByMatrix(expected.vector, a.Rotation, input);
        LegacyCoreMath::MulVecByTranspose(expected.transposed, a.Rotation, input);
        LegacyCoreMath::TransformXform(expected.affine, a, input);
        LegacyCoreMath::MulMatrix(expected.product, a.Rotation, b.Rotation);
        LegacyCoreMath::MulTransform(expected.composition, a, b);
        Candidate(actual, a, b, input);
        FVector vectorAlias = input, transposeAlias = input, affineAlias = input;
        X86MatrixMath::TransformVector(vectorAlias, a.Rotation, vectorAlias);
        X86MatrixMath::TransformTranspose(transposeAlias, a.Rotation, transposeAlias);
        X86TransformMath::Apply(affineAlias, a, affineAlias);
        FMatrix leftAlias = a.Rotation, rightAlias = b.Rotation, transposed = a.Rotation;
        X86MatrixMath::Multiply(leftAlias, leftAlias, b.Rotation);
        X86MatrixMath::Multiply(rightAlias, a.Rotation, rightAlias);
        X86MatrixMath::Transpose(transposed, transposed);
        FTransform composeLeft = a, composeRight = b;
        X86TransformMath::Compose(composeLeft, composeLeft, b);
        X86TransformMath::Compose(composeRight, a, composeRight);
        if (std::memcmp(&vectorAlias, &actual.vector, sizeof(FVector)) ||
            std::memcmp(&transposeAlias, &actual.transposed, sizeof(FVector)) ||
            std::memcmp(&affineAlias, &actual.affine, sizeof(FVector)) ||
            std::memcmp(&leftAlias, &actual.product, sizeof(FMatrix)) ||
            std::memcmp(&rightAlias, &actual.product, sizeof(FMatrix)) ||
            std::memcmp(&composeLeft, &actual.composition, sizeof(FTransform)) ||
            std::memcmp(&composeRight, &actual.composition, sizeof(FTransform))) return 1;
        for (int row = 0; row < 3; ++row) for (int col = 0; col < 3; ++col)
            if (transposed.D[row][col] != a.Rotation.D[col][row]) return 2;
        unsigned short after, status;
        __asm { fnstcw after }
        __asm { fnstsw status }
        if (after != control || (status & 0x3800) || ((_mm_getcsr() ^ originalMxcsr) & ~0x3fu)) return 3;
        __asm { fldcw measurement }
        float oldValues[30], newValues[30];
        std::memcpy(oldValues, &expected, sizeof(expected));
        std::memcpy(newValues, &actual, sizeof(actual));
        for (unsigned index = 0; index < 30; ++index)
        {
            const double error = std::fabs(static_cast<double>(oldValues[index]) - newValues[index]) / std::max(1.0, scales[index]);
            if (!std::isfinite(error) || error > 2e-6)
            {
                std::fprintf(stderr, "Mismatch sample=%u index=%u pc=%x rc=%x old=%g new=%g error=%g\n",
                    sample, index, pc, rc, oldValues[index], newValues[index], error);
                return 4;
            }
            maximum = std::max(maximum, error);
        }
        ++cases;
    }
    __asm { fnclex }
    __asm { fldcw original }
    _mm_setcsr(originalMxcsr);
    std::printf("PASS: %u core cases x five operations; maximum scaled legacy error %.9g (limit 2e-6); input aliases and transpose checked\n", cases, maximum);
    return 0;
}
