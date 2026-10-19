// Standalone comparison of ComputeLightColor in src/Thorn/ThornScene.cpp.
// No engine headers, libraries, game DATA, or game targets are needed.
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <initializer_list>

// Explicit typed handoff of the assembly result, as in Thorn/X87Return.h.
static __declspec(naked) double ReturnSt0()
{
    __asm { ret }
}

// The production kernel's instruction sequence and wrapped integer subtraction.
__declspec(noinline) double AssemblyLightColor(int From, int To, const float* U)
{
    const unsigned int DifferenceBits = static_cast<unsigned int>(To) -
                                        static_cast<unsigned int>(From);
    const int Difference = static_cast<int>(DifferenceBits);
    __asm
    {
        fild dword ptr Difference
        mov eax, U
        fmul dword ptr [eax]
        fiadd dword ptr From
    }
    return ReturnSt0();
}

// Preserve the same wrapped subtraction and evaluation order. Intermediate
// floating-point values have C++ double type; there is no assembly here.
__declspec(noinline) double CppLightColor(int From, int To, const float* U)
{
    const unsigned int DifferenceBits = static_cast<unsigned int>(To) -
                                        static_cast<unsigned int>(From);
    const int Difference = static_cast<int>(DifferenceBits);
    return static_cast<double>(Difference) * static_cast<double>(*U) +
           static_cast<double>(From);
}

static std::uint64_t Bits(double value)
{
    std::uint64_t bits;
    std::memcpy(&bits, &value, sizeof(bits));
    return bits;
}

using Kernel = double (__cdecl*)(int, int, const float*);
struct Extended { unsigned char bytes[10]; };

// Observe the actual x86 ABI return before a C++ double assignment rounds it.
// Only this measurement bridge makes an indirect call inside assembly.
static __declspec(naked) void __cdecl CaptureExtended(Kernel, int, int, const float*, Extended*)
{
    __asm {
        push dword ptr [esp + 10h]
        push dword ptr [esp + 10h]
        push dword ptr [esp + 10h]
        call dword ptr [esp + 10h]
        add esp, 0ch
        mov eax, dword ptr [esp + 14h]
        fstp tbyte ptr [eax]
        ret
    }
}

static std::uint32_t Next(std::uint32_t& state)
{
    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;
    return state;
}

int main()
{
    static_assert(sizeof(void*) == 4, "Run with an x86 compiler environment");
    unsigned short original;
    __asm { fnstcw original }
    // Volatile call targets prevent folding/reusing calls across precision changes.
    double (__cdecl* volatile assembly)(int, int, const float*) = AssemblyLightColor;
    double (__cdecl* volatile cpp)(int, int, const float*) = CppLightColor;
    const unsigned short precisions[] = {0x0000, 0x0200, 0x0300};
    for (unsigned short precision : precisions)
    {
        const unsigned short control = static_cast<unsigned short>((original & ~0x0300) | precision);
        __asm { fldcw control }
        const int width = precision == 0 ? 24 : precision == 0x0200 ? 53 : 64;
        std::printf("\nx87 precision: %d-bit significand\n", width);
        float midpoint = 0.5f;
        volatile double demoAsm = assembly(20, 220, &midpoint);
        volatile double demoCpp = cpp(20, 220, &midpoint);
        std::printf("  Normal midpoint: assembly=%.17g C++=%.17g\n", double(demoAsm), double(demoCpp));
        if (demoAsm != 120 || demoCpp != 120) return 1;
        for (int stress = 0; stress < 2; ++stress)
        {
            std::uint32_t state = 0x829317ab;
            unsigned differences = 0;
            unsigned extendedDifferences = 0;
            for (unsigned sample = 0; sample < 100000; ++sample)
            {
                const int from = stress ? static_cast<int>(Next(state)) : int(Next(state) % 256);
                const int to = stress ? static_cast<int>(Next(state)) : int(Next(state) % 256);
                // Exact float inputs in [0,1], independent of the precision setting.
                float u = static_cast<float>(Next(state) & 0x00ffffffu) / 16777216.0f;
                volatile double a = assembly(from, to, &u);
                volatile double b = cpp(from, to, &u);
                if (Bits(a) != Bits(b))
                {
                    if (differences < 2)
                        std::printf("    from=%d to=%d u=%.9g: assembly=%.17g C++=%.17g\n",
                                    from, to, u, double(a), double(b));
                    ++differences;
                }
                Extended extendedA, extendedB;
                CaptureExtended(assembly, from, to, &u, &extendedA);
                CaptureExtended(cpp, from, to, &u, &extendedB);
                if (std::memcmp(&extendedA, &extendedB, sizeof(Extended)) != 0)
                {
                    if (extendedDifferences < 2)
                    {
                        std::printf("    Extended return differs: from=%d to=%d u=%.9g\n      assembly=", from, to, u);
                        for (int i = 9; i >= 0; --i) std::printf("%02x", extendedA.bytes[i]);
                        std::printf(" C++=");
                        for (int i = 9; i >= 0; --i) std::printf("%02x", extendedB.bytes[i]);
                        std::printf("\n");
                    }
                    ++extendedDifferences;
                }
            }
            std::printf("  %s: double=%u / 100000, extended=%u / 100000 bitwise differences\n",
                        stress ? "Full int32 stress inputs" : "Normal color inputs (0..255)", differences, extendedDifferences);
        }
    }
    __asm { fldcw original }
    return 0;
}
