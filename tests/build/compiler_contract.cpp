#include <windows.h>
#include <float.h>
#include <xmmintrin.h>
#include <cstdio>
#include <cwchar>
#include <cmath>
#include <initializer_list>
#include "x87_trig.h"

// The game relies on the x87 control word changing scalar expression precision.
__declspec(noinline) double ScalarAdd(double a, double b) { return a + b; }
bool Close(double actual, double expected, double tolerance)
{
    // Ordered comparison rejects NaN as well as an incorrect finite result.
    return std::fabs(actual - expected) <= tolerance;
}
#if defined(__clang__)
__attribute__((target("sse")))
#endif
__declspec(noinline) float SseAdd(float a, float b)
{
    return _mm_cvtss_f32(_mm_add_ss(_mm_set_ss(a), _mm_set_ss(b)));
}
int main(int argc, char** argv)
{
    static_assert(sizeof(void*) == 4, "Windows x86 is required");
    unsigned oldWord = 0;
    _controlfp_s(&oldWord, 0, 0);
    unsigned unused = 0;
    // A volatile call target prevents Release from reusing the first result
    // across control-word changes (the two calls have identical arguments).
    double (__cdecl* volatile scalarAdd)(double, double) = ScalarAdd;
    _controlfp_s(&unused, _PC_24, _MCW_PC);
    const double rounded = scalarAdd(16777216.0, 1.0);
    _controlfp_s(&unused, _PC_53, _MCW_PC);
    const double precise = scalarAdd(16777216.0, 1.0);
    const float sse = SseAdd(16777216.0f, 1.0f);
    _controlfp_s(&unused, oldWord, _MCW_PC);
    if (rounded != 16777216.0 || precise != 16777217.0 || sse != 16777216.0f) {
        std::fprintf(stderr, "Floating-point contract failed: %.17g %.17g %.9g\n", rounded, precise, sse);
        return 1;
    }
    // Exercise the real shared assembly helpers through typed calls. Repeating
    // the calls also catches a result left behind on the eight-entry x87 stack.
    const double pi = 3.14159265358979323846;
    unsigned short before, after;
    __asm { fnstsw before }
    for (int repeat = 0; repeat < 32; ++repeat) {
        for (double angle : {-pi / 4, 0.0, pi / 6}) {
            if (!Close(X87Sin(angle), std::sin(angle), 1e-12) ||
                !Close(X87Cos(angle), std::cos(angle), 1e-12) ||
                !Close(X87Tan(angle), std::tan(angle), 1e-12) ||
                !Close(X87Atan(angle), std::atan(angle), 1e-12) ||
                !Close(X87SinF(angle), std::sin(angle), 1e-6) ||
                !Close(X87CosF(angle), std::cos(angle), 1e-6) ||
                !Close(X87TanF(angle), std::tan(angle), 1e-6) ||
                !Close(X87AtanF(angle), std::atan(angle), 1e-6))
                return 5;
        }
        for (double x : {-1.0, 1.0}) for (double y : {-1.0, 1.0}) {
            if (!Close(X87Atan2(y, x), std::atan2(y, x), 1e-12) ||
                !Close(X87Atan2F(y, x), std::atan2(y, x), 1e-6))
                return 6;
        }
    }
    __asm { fnstsw after }
    if ((before & 0x3800) != (after & 0x3800)) return 7;
    if (argc != 2) return 2;
    HMODULE resources = LoadLibraryExA(argv[1], nullptr, LOAD_LIBRARY_AS_DATAFILE);
    if (!resources) return 3;
    wchar_t text[2048];
    bool format = LoadStringW(resources, 1271, text, 2048) > 0 && wcsstr(text, L"%d.%d");
    bool unicode = LoadStringW(resources, 15204, text, 2048) > 0 && wcschr(text, L'\x2014');
    FreeLibrary(resources);
    if (!format || !unicode) {
        std::fprintf(stderr, "Resource format or UTF-8 to UTF-16 compilation failed\n");
        return 4;
    }
    return 0;
}
