// Exercise the real internal kernels without adding test exports to x86math.dll.
#include "../../src/x86math/x86math.cpp"
#include <cstdio>

int main()
{
    const float coefficients[] = {0.0f, 1.0f, -2.0f};
    const FVector vector = {2.0f, -3.0f, 4.0f};
    for (float a : coefficients) for (float b : coefficients) for (float c : coefficients) {
        FFastDot dot;
        BindFastDot(dot, a, b, c);
        float plain = 0, added = 0;
        const float offset = 0.5f;
        StoreKernelPlain(dot, &vector, &plain);
        StoreKernelAdd(dot, &vector, &added, &offset);
        const float expected = a * vector.X + b * vector.Y + c * vector.Z;
        if (plain != expected || added != expected + offset) {
            std::fprintf(stderr, "FastDot mismatch: %g %g %g => %g %g\n", a, b, c, plain, added);
            return 1;
        }
    }
    FMathEngine engine{};
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
    engine.QuatSlerpValue(output, identity, antipodal, &fraction);
    if (output.W != 1 || output.X != 0 || output.Y != 0 || output.Z != 0) return 2;
    engine.QuatSlerpValue(identity, identity, antipodal, &fraction);
    if (identity.W != 1) return 3;
    engine.QuatSlerpValue(antipodal, identity, antipodal, &fraction);
    if (antipodal.W != 1) return 4;
    return 0;
}
