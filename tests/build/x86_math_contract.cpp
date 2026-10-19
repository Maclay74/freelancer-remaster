// Exercise the real internal kernels without adding test exports to x86math.dll.
#include "../../src/x86math/x86math.cpp"
#include <cstdio>

int main()
{
    FMathEngine engine{};
    const FMatrix rotation = {{{0, -1, 0}, {1, 0, 0}, {0, 0, 1}}};
    FVector vectors[] = {{2, -3, 4}, {-1, 5, 6}};
    if (engine.TransformListMatrix(vectors, rotation, vectors, 2) != GR_OK ||
        vectors[0].X != 3 || vectors[0].Y != 2 || vectors[0].Z != 4 ||
        vectors[1].X != -5 || vectors[1].Y != -1 || vectors[1].Z != 6) return 11;
    const FTransform transform = {rotation, {10, 20, 30}};
    FVector translated[2];
    if (engine.TransformListXform(translated, transform, vectors, 2) != GR_OK ||
        translated[0].X != 8 || translated[0].Y != 23 || translated[0].Z != 34 ||
        translated[1].X != 11 || translated[1].Y != 15 || translated[1].Z != 36) return 12;
    float root = 0, inverseRoot = 0;
    if (engine.Sqrt(root, 4.0f) != GR_OK || root != 2.0f ||
        engine.InvSqrt(inverseRoot, 4.0f) != GR_OK || inverseRoot != 0.5f) return 8;
    if (engine.Sqrt(root, 0.0f) != GR_OK || root != 0.0f ||
        engine.InvSqrt(inverseRoot, 0.0f) != GR_OK || !std::isinf(inverseRoot)) return 9;
    FQuaternion identity = {1, 0, 0, 0};
    FMatrix matrix{};
    engine.QuaternionToMatrix(matrix, identity);
    for (int row = 0; row < 3; ++row) for (int col = 0; col < 3; ++col) {
        if (matrix.D[row][col] != (row == col ? 1.0f : 0.0f)) return 5;
    }
    FQuaternion quarterTurn = {0.70710677f, 0.0f, 0.0f, 0.70710677f};
    engine.QuaternionToMatrix(matrix, quarterTurn);
    if (std::fabs(matrix.D[0][0]) > 0.00001f || std::fabs(matrix.D[1][1]) > 0.00001f ||
        std::fabs(std::fabs(matrix.D[0][1]) - 1.0f) > 0.00001f || matrix.D[2][2] != 1.0f) return 6;
    FQuaternion antipodal = {-1, 0, 0, 0};
    FQuaternion output{};
    float fraction = 0.5f;
    if (engine.QuatNlerp(output, identity, antipodal, fraction) != GR_OK || output.W != 1) return 10;
    engine.QuatNlerpValue(output, identity, antipodal, &fraction);
    if (output.W != 1 || output.X != 0 || output.Y != 0 || output.Z != 0) return 2;
    engine.QuatNlerpValue(identity, identity, antipodal, &fraction);
    if (identity.W != 1) return 3;
    engine.QuatNlerpValue(antipodal, identity, antipodal, &fraction);
    if (antipodal.W != 1) return 4;
    return 0;
}
