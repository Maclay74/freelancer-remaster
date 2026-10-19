// Original core x87 kernels retained only for isolated comparisons.
#pragma once
namespace LegacyCoreMath
{
    __declspec(noinline) void MulVecByMatrix(FVector& Dst, const FMatrix& Matrix, const FVector& Source)
    {
        float* ResultVector = &Dst.X;
        const float* MatrixValues = &Matrix.D[0][0];
        const float* SourceVector = &Source.X;
        __asm
        {
            mov ecx, MatrixValues
            mov eax, SourceVector
            mov edx, ResultVector
            fld dword ptr [ecx + 8]
            fmul dword ptr [eax + 8]
            fld dword ptr [ecx + 4]
            fmul dword ptr [eax + 4]
            faddp st(1), st(0)
            fld dword ptr [ecx]
            fmul dword ptr [eax]
            faddp st(1), st(0)
            fstp dword ptr [edx]
            fld dword ptr [ecx + 014h]
            fmul dword ptr [eax + 8]
            fld dword ptr [ecx + 0ch]
            fmul dword ptr [eax]
            faddp st(1), st(0)
            fld dword ptr [ecx + 010h]
            fmul dword ptr [eax + 4]
            faddp st(1), st(0)
            fstp dword ptr [edx + 4]
            fld dword ptr [ecx + 020h]
            fmul dword ptr [eax + 8]
            fld dword ptr [ecx + 018h]
            fmul dword ptr [eax]
            faddp st(1), st(0)
            fld dword ptr [ecx + 01ch]
            fmul dword ptr [eax + 4]
            xor eax, eax
            faddp st(1), st(0)
            fstp dword ptr [edx + 8]
        }
    }

    __declspec(noinline) void MulVecByTranspose(FVector& Dst, const FMatrix& Matrix, const FVector& Source)
    {
        float* ResultVector = &Dst.X;
        const float* MatrixValues = &Matrix.D[0][0];
        const float* SourceVector = &Source.X;
        __asm
        {
            mov ecx, MatrixValues
            mov eax, SourceVector
            mov edx, ResultVector
            fld dword ptr [ecx + 018h]
            fmul dword ptr [eax + 8]
            fld dword ptr [ecx + 0ch]
            fmul dword ptr [eax + 4]
            faddp st(1), st(0)
            fld dword ptr [ecx]
            fmul dword ptr [eax]
            faddp st(1), st(0)
            fstp dword ptr [edx]
            fld dword ptr [ecx + 01ch]
            fmul dword ptr [eax + 8]
            fld dword ptr [ecx + 4]
            fmul dword ptr [eax]
            faddp st(1), st(0)
            fld dword ptr [ecx + 010h]
            fmul dword ptr [eax + 4]
            faddp st(1), st(0)
            fstp dword ptr [edx + 4]
            fld dword ptr [ecx + 020h]
            fmul dword ptr [eax + 8]
            fld dword ptr [ecx + 8]
            fmul dword ptr [eax]
            faddp st(1), st(0)
            fld dword ptr [ecx + 014h]
            fmul dword ptr [eax + 4]
            xor eax, eax
            faddp st(1), st(0)
            fstp dword ptr [edx + 8]
        }
    }

    __declspec(noinline) int MulMatrix(FMatrix& Dst, const FMatrix& A, const FMatrix& B)
    {
        // 71670..716e1: literal x87 loop, preserving product/add/store order
        // and immediate alias visibility between individual result writes.
        float* ResultMatrix = &Dst.D[0][0];
        const float* LeftMatrix = &A.D[0][0];
        const float* RightMatrix = &B.D[0][0];
        __asm
        {
            mov edx, ResultMatrix
            mov ecx, RightMatrix
            mov edi, LeftMatrix
            mov esi, edx
            sub esi, edi
            lea eax, [edi + 4]
            mov edi, 3
        MatrixRowLoop:
            fld dword ptr [eax + 4]
            fmul dword ptr [ecx + 18h]
            fld dword ptr [ecx + 0ch]
            fmul dword ptr [eax]
            add eax, 0ch
            add edx, 0ch
            dec edi
            faddp st(1), st(0)
            fld dword ptr [eax - 10h]
            fmul dword ptr [ecx]
            faddp st(1), st(0)
            fstp dword ptr [edx - 0ch]
            fld dword ptr [ecx + 4]
            fmul dword ptr [eax - 10h]
            fld dword ptr [eax - 8]
            fmul dword ptr [ecx + 1ch]
            faddp st(1), st(0)
            fld dword ptr [ecx + 10h]
            fmul dword ptr [eax - 0ch]
            faddp st(1), st(0)
            fstp dword ptr [esi + eax - 0ch]
            fld dword ptr [ecx + 8]
            fmul dword ptr [eax - 10h]
            fld dword ptr [eax - 8]
            fmul dword ptr [ecx + 20h]
            faddp st(1), st(0)
            fld dword ptr [ecx + 14h]
            fmul dword ptr [eax - 0ch]
            faddp st(1), st(0)
            fstp dword ptr [edx - 4]
            jnz MatrixRowLoop
        }
        return 0;
    }

    __declspec(noinline) int MulTransform(FTransform& Dst, const FTransform& A, const FTransform& B)
    {
        // LAB_06f716f0: literal row/translation interleave and x87 sum order.
        // Input words are read after each output store, including aliased calls.
        float* ResultTransform = &Dst.Rotation.D[0][0];
        const float* LeftTransform = &A.Rotation.D[0][0];
        const float* RightTransform = &B.Rotation.D[0][0];
        __asm
        {
            mov eax, LeftTransform
            mov ecx, RightTransform
            mov edx, ResultTransform
            fld dword ptr [eax]
            fmul dword ptr [ecx]
            fld dword ptr [eax + 4]
            fmul dword ptr [ecx + 0ch]
            faddp st(1), st(0)
            fld dword ptr [ecx + 018h]
            fmul dword ptr [eax + 8]
            faddp st(1), st(0)
            fstp dword ptr [edx]
            fld dword ptr [eax + 4]
            fmul dword ptr [ecx + 010h]
            fld dword ptr [ecx + 01ch]
            fmul dword ptr [eax + 8]
            faddp st(1), st(0)
            fld dword ptr [ecx + 4]
            fmul dword ptr [eax]
            faddp st(1), st(0)
            fstp dword ptr [edx + 4]
            fld dword ptr [eax + 4]
            fmul dword ptr [ecx + 014h]
            fld dword ptr [ecx + 020h]
            fmul dword ptr [eax + 8]
            faddp st(1), st(0)
            fld dword ptr [ecx + 8]
            fmul dword ptr [eax]
            faddp st(1), st(0)
            fstp dword ptr [edx + 8]
            fld dword ptr [eax + 4]
            fmul dword ptr [ecx + 028h]
            fld dword ptr [ecx + 02ch]
            fmul dword ptr [eax + 8]
            faddp st(1), st(0)
            fld dword ptr [ecx + 024h]
            fmul dword ptr [eax]
            faddp st(1), st(0)
            fadd dword ptr [eax + 024h]
            fstp dword ptr [edx + 024h]
            fld dword ptr [eax + 010h]
            fmul dword ptr [ecx + 0ch]
            fld dword ptr [eax + 0ch]
            fmul dword ptr [ecx]
            faddp st(1), st(0)
            fld dword ptr [eax + 014h]
            fmul dword ptr [ecx + 018h]
            faddp st(1), st(0)
            fstp dword ptr [edx + 0ch]
            fld dword ptr [eax + 014h]
            fmul dword ptr [ecx + 01ch]
            fld dword ptr [eax + 010h]
            fmul dword ptr [ecx + 010h]
            faddp st(1), st(0)
            fld dword ptr [eax + 0ch]
            fmul dword ptr [ecx + 4]
            faddp st(1), st(0)
            fstp dword ptr [edx + 010h]
            fld dword ptr [eax + 014h]
            fmul dword ptr [ecx + 020h]
            fld dword ptr [eax + 010h]
            fmul dword ptr [ecx + 014h]
            faddp st(1), st(0)
            fld dword ptr [eax + 0ch]
            fmul dword ptr [ecx + 8]
            faddp st(1), st(0)
            fstp dword ptr [edx + 014h]
            fld dword ptr [ecx + 02ch]
            fmul dword ptr [eax + 014h]
            fld dword ptr [ecx + 028h]
            fmul dword ptr [eax + 010h]
            faddp st(1), st(0)
            fld dword ptr [ecx + 024h]
            fmul dword ptr [eax + 0ch]
            faddp st(1), st(0)
            fadd dword ptr [eax + 028h]
            fstp dword ptr [edx + 028h]
            fld dword ptr [eax + 01ch]
            fmul dword ptr [ecx + 0ch]
            fld dword ptr [eax + 020h]
            fmul dword ptr [ecx + 018h]
            faddp st(1), st(0)
            fld dword ptr [ecx]
            fmul dword ptr [eax + 018h]
            faddp st(1), st(0)
            fstp dword ptr [edx + 018h]
            fld dword ptr [eax + 020h]
            fmul dword ptr [ecx + 01ch]
            fld dword ptr [eax + 01ch]
            fmul dword ptr [ecx + 010h]
            faddp st(1), st(0)
            fld dword ptr [ecx + 4]
            fmul dword ptr [eax + 018h]
            faddp st(1), st(0)
            fstp dword ptr [edx + 01ch]
            fld dword ptr [eax + 020h]
            fmul dword ptr [ecx + 020h]
            fld dword ptr [eax + 01ch]
            fmul dword ptr [ecx + 014h]
            faddp st(1), st(0)
            fld dword ptr [ecx + 8]
            fmul dword ptr [eax + 018h]
            faddp st(1), st(0)
            fstp dword ptr [edx + 020h]
            fld dword ptr [eax + 020h]
            fmul dword ptr [ecx + 02ch]
            fld dword ptr [eax + 01ch]
            fmul dword ptr [ecx + 028h]
            faddp st(1), st(0)
            fld dword ptr [ecx + 024h]
            fmul dword ptr [eax + 018h]
            faddp st(1), st(0)
            fadd dword ptr [eax + 02ch]
            xor eax, eax
            fstp dword ptr [edx + 02ch]
        }
        return 0;
    }

    __declspec(noinline) int TransformXform(FVector& Dst, const FTransform& Xform, const FVector& Source)
    {
        // 71890..718ed: translation is added while each dot remains extended,
        // then that component is stored before the next source read.
        float* ResultVector = &Dst.X;
        const float* TransformValues = &Xform.Rotation.D[0][0];
        const float* SourceVector = &Source.X;
        __asm
        {
            mov eax, TransformValues
            mov ecx, SourceVector
            mov edx, ResultVector
            fld dword ptr [eax + 8]
            fmul dword ptr [ecx + 8]
            fld dword ptr [eax + 4]
            fmul dword ptr [ecx + 4]
            faddp st(1), st(0)
            fld dword ptr [eax]
            fmul dword ptr [ecx]
            faddp st(1), st(0)
            fadd dword ptr [eax + 24h]
            fstp dword ptr [edx]
            fld dword ptr [eax + 14h]
            fmul dword ptr [ecx + 8]
            fld dword ptr [eax + 0ch]
            fmul dword ptr [ecx]
            faddp st(1), st(0)
            fld dword ptr [eax + 10h]
            fmul dword ptr [ecx + 4]
            faddp st(1), st(0)
            fadd dword ptr [eax + 28h]
            fstp dword ptr [edx + 4]
            fld dword ptr [eax + 20h]
            fmul dword ptr [ecx + 8]
            fld dword ptr [eax + 18h]
            fmul dword ptr [ecx]
            faddp st(1), st(0)
            fld dword ptr [eax + 1ch]
            fmul dword ptr [ecx + 4]
            faddp st(1), st(0)
            fadd dword ptr [eax + 2ch]
            fstp dword ptr [edx + 8]
        }
        return 0;
    }
}
