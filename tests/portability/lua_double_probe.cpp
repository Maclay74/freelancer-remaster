#include "../../src/Thorn/LuaTableProbe.h"
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <xmmintrin.h>

struct Trace
{
    char events[4];
    unsigned short statuses[4];
    unsigned count;
    lua_Object table;
    double index;
};
static Trace trace;
static bool valid;
static lua_Object fallback;
static unsigned short callbackControl;

static void Record(char event)
{
    unsigned short status;
    __asm { fnstsw status }
    trace.statuses[trace.count] = status;
    trace.events[trace.count++] = event;
}

__declspec(noinline) void ClearFlags()
{
    __asm {
        fldz
        fstp st(0)
        fnclex
    }
}

extern "C" int lua_istable(lua_Object table)
{
    Record('T');
    trace.table = table;
    fallback = 47; // Failure must read the live fallback after this callback.
    return valid;
}
extern "C" void lua_pushobject(lua_Object table)
{
    Record('O');
    trace.table = table;
    // The conversion must still be exact after a callback changes precision.
    __asm { fldcw callbackControl }
}
extern "C" void lua_pushnumber(double number)
{
    Record('N');
    trace.index = number;
}
extern "C" lua_Object lua_gettable()
{
    Record('G');
    return 99;
}

__declspec(noinline) lua_Object Reference(lua_Object Table, unsigned int Index)
{
#if defined(TEST_FLOAT_INDEX)
    const unsigned long long IntegerIndex = Index;
    float FloatIndex;
    __asm {
        fild qword ptr IntegerIndex
        fstp dword ptr FloatIndex
    }
    if (lua_istable(Table) == 0) return fallback;
    lua_pushobject(Table);
    lua_pushnumber(static_cast<double>(FloatIndex));
    return lua_gettable();
#else
    if (lua_istable(Table) == 0) return fallback;
    lua_pushobject(Table);
    const unsigned long long IntegerIndex = Index;
    double Number;
    __asm {
        fild qword ptr IntegerIndex
        fstp qword ptr Number
    }
    lua_pushnumber(Number);
    return lua_gettable();
#endif
}

__declspec(noinline) lua_Object Candidate(lua_Object table, unsigned int index)
{
#if defined(TEST_FLOAT_INDEX)
    return ThornLuaDetail::GetLightFloatProbe(table, index, fallback);
#else
    return ThornLuaDetail::GetLightDoubleProbe(table, index, fallback);
#endif
}

int main()
{
    unsigned short original;
    __asm { fnstcw original }
    unsigned cases = 0;
    std::uint32_t random = 0x739abc31;
    const unsigned short precisions[] = {0, 0x200, 0x300};
    const unsigned boundaries[] = {0, 1, 2, 3, 0x00ffffff, 0x01000000, 0x01000001,
        0x7fffffff, 0x80000000, 0xfffffffe, 0xffffffff};
    for (unsigned short precision : precisions) for (unsigned short rounding = 0; rounding < 0x1000; rounding += 0x400)
    {
        callbackControl = static_cast<unsigned short>((original & ~0xf00) | precision | rounding);
        for (unsigned sample = 0; sample < 100000; ++sample)
        {
            random ^= random << 13; random ^= random >> 17; random ^= random << 5;
            const unsigned index = sample < 11 ? boundaries[sample] : random;
            for (unsigned isTable = 0; isTable < 2; ++isTable)
            {
                valid = isTable != 0;
                trace = {}; fallback = 11;
                __asm { fldcw callbackControl }
                ClearFlags();
                unsigned short before, expectedStatus, after;
                const unsigned mxcsr = _mm_getcsr();
                __asm { fnstsw before }
                const auto expected = Reference(23, index);
                __asm { fnstsw expectedStatus }
                const Trace expectedTrace = trace;
                trace = {}; fallback = 11;
                ClearFlags();
                const auto actual = Candidate(23, index);
                __asm { fnstsw after }
                if (actual != expected || trace.count != expectedTrace.count || trace.table != expectedTrace.table ||
                    std::memcmp(trace.events, expectedTrace.events, sizeof(trace.events)) ||
                    std::memcmp(trace.statuses, expectedTrace.statuses, sizeof(trace.statuses)) ||
                    std::memcmp(&trace.index, &expectedTrace.index, sizeof(double)) ||
                    expectedStatus != after || mxcsr != _mm_getcsr())
                {
                    std::fprintf(stderr, "Mismatch index=%u table=%u pc=%x rounding=%x\n", index, isTable, precision, rounding);
                    std::fprintf(stderr, "index expected %.17g actual %.17g; status %x -> %x; callbacks %u -> %u\n",
                                 expectedTrace.index, trace.index, before, after, expectedTrace.count, trace.count);
                    return 1;
                }
                ++cases;
            }
        }
    }
    __asm { fldcw original }
    std::printf("PASS: %u exact return/callback/index and x87 status comparisons; precision/rounding modes; MXCSR unchanged\n", cases);
    return 0;
}
