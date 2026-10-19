// Original x87 conversion kernels retained only for isolated comparisons.
#pragma once
namespace LegacyQuaternionMath
{
    __declspec(noinline) int MatrixToQuaternion(FQuaternion& Dst, const FMatrix& Source)
    {
        const FMatrix* SourceMatrix = &Source;
        FQuaternion* DestinationQuat = &Dst;
        const float Zero = 0.0f;
        const float One = 1.0f;
        const float Half = 0.5f;
        static const int NextAxis[3] = {1, 2, 0};
        // LAB_06f72290..723b1 keeps the trace, root and reciprocal on x87;
        // only the four destination writes round to float. AND EAX,4100/JNZ
        // takes the not-greater (including unordered) branch in all three tests.
        // Keep the branch-specific store/read order for aliased source/output.
        __asm
        {
            mov ecx, SourceMatrix
            mov edi, DestinationQuat
            fld dword ptr [ecx + 10h]
            fadd dword ptr [ecx]
            fadd dword ptr [ecx + 20h]
            fcom Zero
            fnstsw ax
            and eax, 4100h
            jnz MatrixQuatNotPositive
            fadd One
            fsqrt
            fld Half
            fmul st(0), st(1)
            fstp dword ptr [edi]
            fdivr Half
            fld dword ptr [ecx + 1ch]
            fsub dword ptr [ecx + 14h]
            fmul st(0), st(1)
            fstp dword ptr [edi + 4]
            fld dword ptr [ecx + 8]
            fsub dword ptr [ecx + 18h]
            fmul st(0), st(1)
            fstp dword ptr [edi + 8]
            fld dword ptr [ecx + 0ch]
            fsub dword ptr [ecx + 4]
            fmul st(0), st(1)
            fstp dword ptr [edi + 0ch]
            fstp st(0)
            jmp MatrixQuatDone
        MatrixQuatNotPositive:
            fstp st(0)
            fld dword ptr [ecx + 10h]
            fcomp dword ptr [ecx]
            xor edx, edx
            fnstsw ax
            and eax, 4100h
            jnz MatrixQuatSecondDiagonal
            mov edx, 1
        MatrixQuatSecondDiagonal:
            fld dword ptr [ecx + 20h]
            mov eax, edx
            shl eax, 4
            fcomp dword ptr [ecx + eax]
            fnstsw ax
            and eax, 4100h
            jnz MatrixQuatIndices
            mov edx, 2
        MatrixQuatIndices:
            mov eax, dword ptr NextAxis[edx * 4]
            mov esi, dword ptr NextAxis[eax * 4]
            mov ebx, esi
            shl ebx, 4
            fld dword ptr [ecx + ebx]
            mov ebx, eax
            shl ebx, 4
            fadd dword ptr [ecx + ebx]
            mov ebx, edx
            shl ebx, 4
            fsubr dword ptr [ecx + ebx]
            fadd One
            fsqrt
            fld Half
            fmul st(0), st(1)
            fstp dword ptr [edi + edx * 4 + 4]
            fdivr Half
            lea ebx, [esi + esi * 2]
            add ebx, eax
            fld dword ptr [ecx + ebx * 4]
            lea ebx, [eax + eax * 2]
            add ebx, esi
            fsub dword ptr [ecx + ebx * 4]
            fmul st(0), st(1)
            fstp dword ptr [edi]
            lea ebx, [edx + edx * 2]
            add ebx, eax
            fld dword ptr [ecx + ebx * 4]
            lea ebx, [eax + eax * 2]
            add ebx, edx
            fadd dword ptr [ecx + ebx * 4]
            fmul st(0), st(1)
            fstp dword ptr [edi + eax * 4 + 4]
            lea ebx, [edx + edx * 2]
            add ebx, esi
            fld dword ptr [ecx + ebx * 4]
            lea ebx, [esi + esi * 2]
            add ebx, edx
            fadd dword ptr [ecx + ebx * 4]
            fmul st(0), st(1)
            fstp dword ptr [edi + esi * 4 + 4]
            fstp st(0)
        MatrixQuatDone:
        }
        return 0;
    }

    __declspec(noinline) int QuaternionToMatrix(FMatrix& Dst, const FQuaternion& Quat)
    {
        // 723c0..724c4: retain S/Xs and selected products on x87; the
        // asymmetric float scratch stores are part of the arithmetic contract.
        float Scratch[5];
        float Zs;
        const float One = 1.0f;
        const float Two = 2.0f;
        // Named reference operands have different MSVC/Clang inline-asm
        // semantics. Explicit pointers keep both compilers on the same ABI.
        const FQuaternion* const QuatPtr = &Quat;
        FMatrix* const DstPtr = &Dst;
        __asm
        {
            mov eax, QuatPtr
            lea edx, Scratch
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
            fdivr Two
            fstp st(4)
            fstp st(0)
            fstp st(0)
            fstp st(0)
            fld st(0)
            fmul dword ptr [eax + 4]
            fld st(1)
            fmul dword ptr [eax + 8]
            fstp dword ptr [edx]
            fxch st(1)
            fmul dword ptr [eax + 0ch]
            fstp Zs
            fld st(0)
            fmul dword ptr [eax]
            fstp dword ptr [edx + 0ch]
            fld dword ptr [edx]
            fmul dword ptr [eax]
            fstp dword ptr [edx + 8]
            fld Zs
            fmul dword ptr [eax]
            fstp dword ptr [edx + 4]
            fmul dword ptr [eax + 4]
            fld dword ptr [edx]
            fmul dword ptr [eax + 4]
            fld Zs
            fmul dword ptr [eax + 4]
            fld dword ptr [edx]
            fmul dword ptr [eax + 8]
            fstp dword ptr [edx + 10h]
            fld Zs
            fmul dword ptr [eax + 8]
            fstp dword ptr [edx]
            fld Zs
            fmul dword ptr [eax + 0ch]
            fld dword ptr [edx + 10h]
            // Every quaternion input read precedes the first output write.
            mov eax, DstPtr
            fadd st(0), st(1)
            fsubr One
            fstp dword ptr [eax]
            fld st(2)
            fsub dword ptr [edx + 4]
            fstp dword ptr [eax + 4]
            fld st(1)
            fadd dword ptr [edx + 8]
            fstp dword ptr [eax + 8]
            fxch st(2)
            fadd dword ptr [edx + 4]
            fstp dword ptr [eax + 0ch]
            fxch st(1)
            fadd st(0), st(2)
            fsubr One
            fstp dword ptr [eax + 10h]
            fld dword ptr [edx]
            fsub dword ptr [edx + 0ch]
            fstp dword ptr [eax + 14h]
            fsub dword ptr [edx + 8]
            fstp dword ptr [eax + 18h]
            fld dword ptr [edx]
            fadd dword ptr [edx + 0ch]
            fstp dword ptr [eax + 1ch]
            fld dword ptr [edx + 10h]
            fadd st(0), st(1)
            fsubr One
            fstp dword ptr [eax + 20h]
            fstp st(0)
        }
        return 0;
    }
}
