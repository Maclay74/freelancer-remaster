#include "../../src/Thorn/lua32/src/lib/PortableMath.h"
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <limits>

static double arguments[2], result;
static double luaL_check_number(int index) { return arguments[index - 1]; }
static void lua_pushnumber(double value) { result = value; }
#include "candidates/legacy_lua_math.h"

using Unary = double (*)(double);
__declspec(noinline) static double Standard(unsigned op, double x, double y)
{
    const Unary unary[] = {ThornLuaSin, ThornLuaCos, ThornLuaTan, ThornLuaAtan,
        nullptr, ThornLuaSqrt, ThornLuaLog, ThornLuaLog10, ThornLuaExp};
    return op == 4 ? ThornLuaAtan2(x, y) : unary[op](x);
}
__declspec(noinline) static double Legacy(unsigned op, double x, double y)
{
    arguments[0] = x; arguments[1] = y;
    void (*const funcs[])() = {LegacyLuaMath::math_sin, LegacyLuaMath::math_cos,
        LegacyLuaMath::math_tan, LegacyLuaMath::math_atan, LegacyLuaMath::math_atan2,
        LegacyLuaMath::math_sqrt, LegacyLuaMath::math_log, LegacyLuaMath::math_log10,
        LegacyLuaMath::math_exp};
    funcs[op]();
    return result;
}

int main()
{
    unsigned short original;
    __asm { fnstcw original }
    const unsigned short measurement = static_cast<unsigned short>((original & ~0xf00) | 0x200 | 0x3f);
    const unsigned short precisions[] = {0, 0x200, 0x300};
    unsigned failures = 0, cases = 0;
    double maximum[9] = {};
    std::uint32_t random = 0x1b571;
    for (unsigned short pc : precisions) for (unsigned short rc = 0; rc < 0x1000; rc += 0x400)
        for (unsigned sample = 0; sample < 2048; ++sample)
    {
        __asm { fldcw measurement }
        random ^= random << 13; random ^= random >> 17; random ^= random << 5;
        const double base = (static_cast<int>(random % 1474561) - 737280) / 1024.0;
        const double y = (static_cast<int>((random >> 4) % 16385) - 8192) / 1024.0;
        for (unsigned op = 0; op < 9; ++op) {
            __asm { fldcw measurement }
            const double x = op >= 5 && op <= 7 ? std::fabs(base) + 0.00001 :
                op == 8 ? base / 24.0 : base;
            if (op == 2 && std::fabs(std::cos(x * 0.017453292519943295)) < 0.01) continue;
            const unsigned short control = static_cast<unsigned short>((original & ~0xf00) | pc | rc | 0x3f);
            __asm { fldcw control }
            const double old = Legacy(op,x,y);
            const double now = Standard(op,x,y);
            __asm { fldcw measurement }
            const double error = std::fabs(now-old)/(1+std::fabs(old));
            if (error > maximum[op]) maximum[op] = error;
            const double tolerance = pc == 0 && op == 8 ? 4e-6 :
                pc == 0 && op == 5 ? 2e-7 : 1e-11;
            if (!(error <= tolerance) && failures++ < 8)
                std::printf("Lua math mismatch pc=%x rc=%x op=%u x=%.17g y=%.17g old=%.17g new=%.17g error=%g\n",
                    pc,rc,op,x,y,old,now,error);
            ++cases;
        }
    }
    __asm { fldcw measurement }
    const double inf = std::numeric_limits<double>::infinity();
    if (!std::isfinite(Standard(0,1e20,0)) || Standard(8,-inf,0) != 0 ||
        Standard(8,inf,0) != inf || !std::isnan(Standard(5,-1,0)) ||
        Standard(6,0,0) != -inf || !std::signbit(Standard(0,-0.0,0))) ++failures;
    __asm { fnclex }
    __asm { fldcw original }
    std::printf("%u Lua math comparisons; %u failures; max scaled errors:",cases,failures);
    for (double value : maximum) std::printf(" %.3g",value);
    std::puts("");
    return failures ? 1 : 0;
}
