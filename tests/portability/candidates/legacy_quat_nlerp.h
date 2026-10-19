// Original x87 implementation, retained only as a test oracle.
#pragma once
    __declspec(noinline) void LegacyNlerp(Quaternion& Dst, const Quaternion& A, const Quaternion& B, const float* T)
    {
        // 724d0..725cb: normalized linear interpolation, literal x87 terms,
        // component store order and NaN hemisphere selection. Inputs may alias Dst.
        static const float Zero = 0.0f;
        static const float One = 1.0f;
        const void* const Args[] = {T, &A, &B, &Dst, &Zero, &One};
        __asm
        {
            lea edi, Args
            mov esi, dword ptr [edi]
            mov ecx, dword ptr [edi + 4]
            mov edx, dword ptr [edi + 8]
            fld dword ptr [ecx + 8]
            fmul dword ptr [edx + 8]
            fld dword ptr [ecx + 4]
            fmul dword ptr [edx + 4]
            faddp st(1), st(0)
            fld dword ptr [edx + 0ch]
            fmul dword ptr [ecx + 0ch]
            faddp st(1), st(0)
            fld dword ptr [ecx]
            fmul dword ptr [edx]
            faddp st(1), st(0)
            mov ebx, dword ptr [edi + 16]
            fcomp dword ptr [ebx]
            fnstsw ax
            test ah, 05h
            mov eax, dword ptr [edi + 12]
            // Less=01h (odd parity) falls through. Nonnegative=00h and
            // unordered=05h (even parity) take the forward branch.
            jp QuatForwardHemisphere
            fld dword ptr [ecx]
            fadd dword ptr [edx]
            fmul dword ptr [esi]
            fsubr dword ptr [ecx]
            fstp dword ptr [eax]
            fld dword ptr [ecx + 4]
            fadd dword ptr [edx + 4]
            fmul dword ptr [esi]
            fsubr dword ptr [ecx + 4]
            fstp dword ptr [eax + 4]
            fld dword ptr [ecx + 8]
            fadd dword ptr [edx + 8]
            fmul dword ptr [esi]
            fsubr dword ptr [ecx + 8]
            fstp dword ptr [eax + 8]
            fld dword ptr [edx + 0ch]
            fadd dword ptr [ecx + 0ch]
            fmul dword ptr [esi]
            fsubr dword ptr [ecx + 0ch]
            jmp QuatNormalize
        QuatForwardHemisphere:
            fld dword ptr [edx]
            fsub dword ptr [ecx]
            fmul dword ptr [esi]
            fadd dword ptr [ecx]
            fstp dword ptr [eax]
            fld dword ptr [edx + 4]
            fsub dword ptr [ecx + 4]
            fmul dword ptr [esi]
            fadd dword ptr [ecx + 4]
            fstp dword ptr [eax + 4]
            fld dword ptr [edx + 8]
            fsub dword ptr [ecx + 8]
            fmul dword ptr [esi]
            fadd dword ptr [ecx + 8]
            fstp dword ptr [eax + 8]
            fld dword ptr [edx + 0ch]
            fsub dword ptr [ecx + 0ch]
            fmul dword ptr [esi]
            fadd dword ptr [ecx + 0ch]
        QuatNormalize:
            fstp dword ptr [eax + 0ch]
            fld dword ptr [eax + 0ch]
            fld dword ptr [eax + 8]
            fld dword ptr [eax + 4]
            fld dword ptr [eax]
            fld st(0)
            fmul st(0), st(1)
            fld st(2)
            fmul st(0), st(3)
            faddp st(1), st(0)
            fld st(3)
            fmul st(0), st(4)
            faddp st(1), st(0)
            fld st(4)
            fmul st(0), st(5)
            faddp st(1), st(0)
            fsqrt
            fstp st(4)
            fstp st(0)
            fstp st(0)
            fstp st(0)
            mov ebx, dword ptr [edi + 20]
            fdivr dword ptr [ebx]
            fld st(0)
            fmul dword ptr [eax]
            fstp dword ptr [eax]
            fld st(0)
            fmul dword ptr [eax + 4]
            fstp dword ptr [eax + 4]
            fld st(0)
            fmul dword ptr [eax + 8]
            fstp dword ptr [eax + 8]
            fmul dword ptr [eax + 0ch]
            fstp dword ptr [eax + 0ch]
        }
    }
