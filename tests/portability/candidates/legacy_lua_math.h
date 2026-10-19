#pragma once

// Test-only original Lua math kernels; hooks capture arguments and output.
namespace LegacyLuaMath
{
__declspec(noinline) static void math_sin (void) {
  const double Input = luaL_check_number(1);
  const unsigned long long ScaleBits = 0x3f91df46a2529d39ULL;
  double Scale;
  double Result;
  memcpy(&Scale, &ScaleBits, sizeof(Scale));
  /* 56540: the compiled C/intrinsic candidates reduce huge inputs, while
     retail FSIN leaves out-of-range operands unchanged (C2). */
  __asm {
    fld Input
    fmul Scale
    fsin
    fstp Result
  }
  lua_pushnumber(Result);
}

__declspec(noinline) static void math_cos (void) {
  const double Input = luaL_check_number(1);
  const unsigned long long ScaleBits = 0x3f91df46a2529d39ULL;
  double Scale;
  double Result;
  memcpy(&Scale, &ScaleBits, sizeof(Scale));
  /* 56560: native C/intrinsic FCOS policy differs on finite and C2 inputs. */
  __asm {
    fld Input
    fmul Scale
    fcos
    fstp Result
  }
  lua_pushnumber(Result);
}

__declspec(noinline) static void math_tan (void) {
  const double Input = luaL_check_number(1);
  const unsigned long long ScaleBits = 0x3f91df46a2529d39ULL;
  double Scale;
  double Result;
  memcpy(&Scale, &ScaleBits, sizeof(Scale));
  __asm {
    fld Input
    fmul Scale
    fptan
    fstp st(0)
    fstp Result
  }
  lua_pushnumber(Result);
}

__declspec(noinline) static void math_atan (void) {
  const double Input = luaL_check_number(1);
  const unsigned long long ScaleBits = 0x404ca5dc1a63c1f8ULL;
  double Scale;
  double Result;
  memcpy(&Scale, &ScaleBits, sizeof(Scale));
  /* Native C/intrinsic atan differs from this owned FPATAN at PC24. */
  __asm {
    fld Input
    fld1
    fpatan
    fmul Scale
    fstp Result
  }
  lua_pushnumber(Result);
}

__declspec(noinline) static void math_atan2 (void) {
  const double First = luaL_check_number(1);
  const double Second = luaL_check_number(2);
  const unsigned long long ScaleBits = 0x404ca5dc1a63c1f8ULL;
  double Scale;
  double Result;
  memcpy(&Scale, &ScaleBits, sizeof(Scale));
  __asm {
    fld First
    fld Second
    fpatan
    fmul Scale
    fstp Result
  }
  lua_pushnumber(Result);
}

__declspec(noinline) static void math_sqrt (void) {
  const double Input = luaL_check_number(1);
  double Result;
  /* Current compiled sqrt/intrinsic differs from owned FSQRT under RC_DOWN. */
  __asm {
    fld Input
    fsqrt
    fstp Result
  }
  lua_pushnumber(Result);
}

__declspec(noinline) static void math_log (void) {
  const double Input = luaL_check_number(1);
  double Result;
  __asm {
    fld Input
    fldln2
    fxch st(1)
    fyl2x
    fstp Result
  }
  lua_pushnumber(Result);
}

__declspec(noinline) static void math_log10 (void) {
  const double Input = luaL_check_number(1);
  double Result;
  __asm {
    fld Input
    fldlg2
    fxch st(1)
    fyl2x
    fstp Result
  }
  lua_pushnumber(Result);
}

__declspec(noinline) static void math_exp (void) {
  const double Input = luaL_check_number(1);
  double Result;
  __asm {
    fld Input
    fldl2e
    fmulp st(1), st(0)
    fld st(0)
    frndint
    fxch st(1)
    fsub st(0), st(1)
    f2xm1
    fld1
    faddp st(1), st(0)
    fscale
    fstp st(1)
    fstp Result
  }
  lua_pushnumber(Result);
}
}
