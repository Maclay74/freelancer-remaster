#include <windows.h>
#include <float.h>
#include <xmmintrin.h>
#include <cstdio>
#include <cwchar>

// The game relies on the x87 control word changing scalar expression precision.
__declspec(noinline) double ScalarAdd(double a, double b) { return a + b; }
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
