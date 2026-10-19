#pragma once
namespace LegacyLuaBinary
{
__declspec(noinline) static void Subtract(double* LeftNumber, const double* RightNumber)
{
    __asm {
              mov eax, RightNumber
              mov ecx, LeftNumber
              fld qword ptr [eax]
              fsubr qword ptr [ecx]
              fstp qword ptr [ecx]
            }
}
__declspec(noinline) static void Divide(double* LeftNumber, const double* RightNumber)
{
    __asm {
              mov eax, RightNumber
              mov ecx, LeftNumber
              fld qword ptr [eax]
              fdivr qword ptr [ecx]
              fstp qword ptr [ecx]
            }
}
}
