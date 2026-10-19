#pragma once

#include "thorn_iface.h"

#if defined(__clang__) && defined(_M_IX86)
// Clang does not treat an ordinary function's final MS-style asm FLD as its
// C++ return value. These explicit call results preserve ST(0), including the
// extended precision used internally, without an intermediate float store.
[[maybe_unused]] static __declspec(naked) float __cdecl ThornReturnExtendedFloat(const FThornExtendedProgress*)
{
    __asm {
        mov eax, dword ptr [esp + 4]
        fld tbyte ptr [eax]
        ret
    }
}
// The arithmetic kernels already own a result in ST(0). Give Clang an
// explicit typed return without rounding it through float/double storage.
[[maybe_unused]] static __declspec(naked) float __cdecl ThornReturnSt0Float()
{
    __asm { ret }
}
[[maybe_unused]] static __declspec(naked) double __cdecl ThornReturnSt0Double()
{
    __asm { ret }
}
#endif
