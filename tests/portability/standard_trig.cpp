// Compare the standard expressions now used directly at game call sites with
// the removed instruction wrappers. There is intentionally no production wrapper.
#include "candidates/legacy_x87_trig.h"
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <limits>
#include <xmmintrin.h>

__declspec(noinline) static double Legacy(unsigned operation, double x, double y)
{
    switch (operation) {
    case 0: return X87Sin(x);
    case 1: return X87Cos(x);
    case 2: return X87Tan(x);
    case 3: return X87Atan(x);
    case 4: return X87Atan2(y, x);
    case 5: return X87SinF(x);
    case 6: return X87CosF(x);
    case 7: return X87TanF(x);
    case 8: return X87AtanF(x);
    default: return X87Atan2F(y, x);
    }
}

__declspec(noinline) static double Standard(unsigned operation, double x, double y)
{
    switch (operation) {
    case 0: return std::sin(x);
    case 1: return std::cos(x);
    case 2: return std::tan(x);
    case 3: return std::atan(x);
    case 4: return std::atan2(y, x);
    case 5: return static_cast<float>(std::sin(x));
    case 6: return static_cast<float>(std::cos(x));
    case 7: return static_cast<float>(std::tan(x));
    case 8: return static_cast<float>(std::atan(x));
    default: return static_cast<float>(std::atan2(y, x));
    }
}

int main()
{
    unsigned short original;
    __asm { fnstcw original }
    const unsigned mxcsr = _mm_getcsr();
    const unsigned short measurement = static_cast<unsigned short>((original & ~0xf00) | 0x200 | 0x3f);
    const double pi = 3.14159265358979323846;
    const double boundary[] = {0.0, -0.0, pi, -pi, pi / 2, -pi / 2,
        pi / 4, -pi / 4, pi / 2 - 0.0001, pi / 2 + 0.0001,
        1e-30, -1e-30, 4096.0, -4096.0};
    const unsigned short precisions[] = {0, 0x200, 0x300};
    std::uint32_t random = 0x6f310bu;
    unsigned comparisons = 0, failures = 0;
    double maximum[10] = {};
    for (unsigned short precision : precisions) {
        const unsigned short control = static_cast<unsigned short>((original & ~0xf00) | precision | 0x3f);
        for (unsigned sample = 0; sample < 8192; ++sample) {
            __asm { fldcw measurement }
            random ^= random << 13; random ^= random >> 17; random ^= random << 5;
            const double x = sample < sizeof(boundary) / sizeof(*boundary) ? boundary[sample] :
                (static_cast<int>(random % 4194305) - 2097152) / 512.0;
            const double y = sample < 4 ? boundary[sample & 1] :
                (static_cast<int>((random >> 4) % 8193) - 4096) / 512.0;
            for (unsigned operation = 0; operation < 10; ++operation) {
                // Tangent is ill-conditioned at its poles: do not assert legacy
                // agreement there. Standard math's range reduction is deliberate.
                if ((operation == 2 || operation == 7) && std::fabs(std::cos(x)) < 1e-5)
                    continue;
                __asm { fldcw control }
                const double legacy = Legacy(operation, x, y);
                const double standard = Standard(operation, x, y);
                __asm { fldcw measurement }
                const double error = std::fabs(standard - legacy) / (1.0 + std::fabs(legacy));
                if (error > maximum[operation]) maximum[operation] = error;
                const double tolerance = operation >= 5 ? 2e-6 : operation == 2 ? 1e-10 : 1e-12;
                if (!(error <= tolerance)) {
                    if (failures++ < 8)
                        std::printf("Mismatch pc=%x op=%u x=%.17g y=%.17g old=%.17g new=%.17g error=%g\n",
                            precision, operation, x, y, legacy, standard, error);
                }
                ++comparisons;
            }
        }
    }
    __asm { fldcw measurement }
    // Standard range reduction remains defined beyond the x87 instruction range.
    const double huge = Standard(0, 1e20, 0);
    if (!std::isfinite(huge) || std::fabs(huge) > 1.0 ||
        !std::signbit(Standard(0, -0.0, 0)) ||
        !std::isnan(Standard(0, std::numeric_limits<double>::infinity(), 0)) ||
        !std::isnan(Standard(1, std::numeric_limits<double>::quiet_NaN(), 0))) ++failures;
    if (_mm_getcsr() != mxcsr) {
        // Standard math may raise status flags. Its SSE controls must stay put.
        if ((_mm_getcsr() & ~0x3fu) != (mxcsr & ~0x3fu)) ++failures;
    }
    __asm { fnclex }
    __asm { fldcw original }
    _mm_setcsr(mxcsr);
    std::printf("%u comparisons; %u failures; max scaled errors:", comparisons, failures);
    for (double error : maximum) std::printf(" %.3g", error);
    std::puts("");
    return failures ? 1 : 0;
}
