// Compile the real spline helpers; /Gy + /OPT:REF discard unrelated engine code.
#include "../../src/Thorn/SplinePath.cpp"
#include <cstdio>
I3DMathEngine* ThornAcquire3DMathEngine() { std::abort(); }
bool ThornNameEqual(const char* a, const char* b) { return a && b && _stricmp(a, b) == 0; }
int main()
{
    FQuat source = {1, 2, 3, 4}, scaled{};
    QuatScale(scaled, source, 2);
    if (scaled.W != 2 || scaled.X != 4 || scaled.Y != 6 || scaled.Z != 8) return 1;
    float scale = 0;
    QuatInverseScaleRetail(source, scale);
    if (std::fabs(scale - 1.0f / 30.0f) > 0.000001f) return 2;
    FVec3 vector = {3, 0, 4};
    NormalizeLookAtVector(vector);
    if (std::fabs(vector.X - 0.6f) > 0.000001f || std::fabs(vector.Z - 0.8f) > 0.000001f) return 3;
    FVec3 a = {1, 0, 0}, b = {0, 1, 0}, cross{};
    CrossLookAtVector(cross, a, b, false);
    // The first retail branch computes B x A; the reverse branch computes A x B.
    if (cross.X != 0 || cross.Y != 0 || cross.Z != -1) return 4;
    CrossLookAtVector(cross, a, b, true);
    if (cross.X != 0 || cross.Y != 0 || cross.Z != 1) return 6;
    FQuat sum{};
    QuatAdd(sum, source, scaled);
    if (sum.W != 3 || sum.X != 6 || sum.Y != 9 || sum.Z != 12) return 5;
    // Loading a position path exercises the ghost-vector assembly that ran
    // during the startup camera script and previously dereferenced float bits.
    FSplinePath path(false);
    const float points[] = {0, 0, 0, 1, 2, 3, 2, 4, 6};
    if (path.Slot4LoadPoints(points, 3) != GR_OK) return 7;
    float time = 0, position[3] = {-1, -1, -1};
    path.EvaluatePosition(&time, position);
    if (position[0] != 0 || position[1] != 0 || position[2] != 0) return 8;
    return 0;
}
