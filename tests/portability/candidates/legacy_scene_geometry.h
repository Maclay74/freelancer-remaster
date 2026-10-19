#pragma once

namespace LegacySceneGeometry
{
    __declspec(noinline) void Cross3(float* Result, const float* A, const float* B)
    {
        __asm
        {
            mov ecx, A
            mov eax, B
            fld dword ptr [ecx]
            fmul dword ptr [eax+4]
            fld dword ptr [ecx+4]
            fmul dword ptr [eax]
            fsubp st(1), st(0)
            fld dword ptr [ecx+8]
            fmul dword ptr [eax]
            fld dword ptr [eax+8]
            fmul dword ptr [ecx]
            fsubp st(1), st(0)
            fld dword ptr [ecx+4]
            fmul dword ptr [eax+8]
            fld dword ptr [ecx+8]
            fmul dword ptr [eax+4]
            mov eax, Result
            fsubp st(1), st(0)
            fstp dword ptr [eax]
            fstp dword ptr [eax+4]
            fstp dword ptr [eax+8]
        }
    }

    __declspec(noinline) void Normalize3(float* V)
    {
        const float One = 1.0f;
        __asm
        {
            mov ecx, V
            fld dword ptr [ecx+8]
            fld dword ptr [ecx+4]
            fld dword ptr [ecx]
            fld st(0)
            fmul st(0), st(1)
            fld st(2)
            fmul st(0), st(3)
            faddp st(1), st(0)
            fld st(3)
            fmul st(0), st(4)
            faddp st(1), st(0)
            fsqrt
            fstp st(3)
            fstp st(0)
            fstp st(0)
            fdivr dword ptr One
            mov eax, V
            fld st(0)
            fmul dword ptr [eax]
            fstp dword ptr [eax]
            fld st(0)
            fmul dword ptr [eax+4]
            fstp dword ptr [eax+4]
            fmul dword ptr [eax+8]
            fstp dword ptr [eax+8]
        }
    }

    __declspec(noinline) void NormalizeAxis3(float* V)
    {
        const float One = 1.0f;
        __asm
        {
            mov ecx, V
            fld dword ptr [ecx+8]
            fld dword ptr [ecx+4]
            fld dword ptr [ecx]
            fld st(0)
            fmul st(0), st(1)
            fld st(2)
            fmul st(0), st(3)
            faddp st(1), st(0)
            fld st(3)
            fmul st(0), st(4)
            faddp st(1), st(0)
            fsqrt
            fstp st(3)
            fstp st(0)
            fstp st(0)
            fdivr dword ptr One
            mov eax, V
            fld dword ptr [eax]
            fmul st(0), st(1)
            fstp dword ptr [eax]
            fld dword ptr [eax+4]
            fmul st(0), st(1)
            fstp dword ptr [eax+4]
            fld dword ptr [eax+8]
            fmul st(0), st(1)
            fstp dword ptr [eax+8]
            fstp st(0)
        }
    }

    __declspec(noinline) void CrossAxisSide3(float* Result, const float* Up, const float* Front, bool bInitial)
    {
        const float* LastLeft = bInitial ? Front + 1 : Up + 2;
        const float* LastRight = bInitial ? Up + 2 : Front + 1;
        __asm
        {
            mov ecx, Up
            mov edx, Front
            mov eax, Result
            fld dword ptr [edx+4]
            fmul dword ptr [ecx]
            fld dword ptr [ecx+4]
            fmul dword ptr [edx]
            fsubp st(1), st(0)
            fstp dword ptr [eax+8]
            fld dword ptr [ecx+8]
            fmul dword ptr [edx]
            fld dword ptr [edx+8]
            fmul dword ptr [ecx]
            fsubp st(1), st(0)
            fstp dword ptr [eax+4]
            fld dword ptr [edx+8]
            fmul dword ptr [ecx+4]
            mov ecx, LastLeft
            mov edx, LastRight
            fld dword ptr [ecx]
            fmul dword ptr [edx]
            fsubp st(1), st(0)
            fstp dword ptr [eax]
        }
    }

    __declspec(noinline) void CrossAxisUp3(float* Result, const float* Dir, const float* Side)
    {
        __asm
        {
            mov ecx, Dir
            mov edx, Side
            mov eax, Result
            fld dword ptr [edx+4]
            fmul dword ptr [ecx]
            fld dword ptr [ecx+4]
            fmul dword ptr [edx]
            fsubp st(1), st(0)
            fstp dword ptr [eax+8]
            fld dword ptr [ecx+8]
            fmul dword ptr [edx]
            fld dword ptr [edx+8]
            fmul dword ptr [ecx]
            fsubp st(1), st(0)
            fstp dword ptr [eax+4]
            fld dword ptr [edx+8]
            fmul dword ptr [ecx+4]
            fld dword ptr [edx+4]
            fmul dword ptr [ecx+8]
            fsubp st(1), st(0)
            fstp dword ptr [eax]
        }
    }

    __declspec(noinline) bool AxisDotNotGreater(const float* Up, const float* Dir)
    {
        // Raw DAT_06f5a248 = 3fefae147ae147ae (double 0.99).
        const double Threshold = 0.99;
        unsigned char bNotGreater;
        __asm
        {
            mov ecx, Up
            mov edx, Dir
            fld dword ptr [ecx+8]
            fmul dword ptr [edx+8]
            fld dword ptr [ecx+4]
            fmul dword ptr [edx+4]
            faddp st(1), st(0)
            fld dword ptr [ecx]
            fmul dword ptr [edx]
            faddp st(1), st(0)
            fabs
            fcomp qword ptr Threshold
            fnstsw ax
            test ah, 41h
            setnz bNotGreater
        }
        // TEST AH41/JNZ selects not-greater, including unordered; greater
        // falls through to the near-parallel fallback in the retail listing.
        return bNotGreater != 0;
    }

    __declspec(noinline) void NormalizeLookAtVector(FVec3& Vector)
    {
        const float One = 1.0f;
        // The intrinsic C++ candidate changes PC24/RC_DOWN output bits and
        // spills Inverse to qword in Debug. 341cc..34215 retains it through
        // all three float stores; 342b5..342f7 uses the same arithmetic.
        FVec3* const VectorPtr = &Vector;
        __asm
        {
            mov edx, VectorPtr
            fld dword ptr [edx + 8]
            fmul dword ptr [edx + 8]
            fld dword ptr [edx + 4]
            fmul dword ptr [edx + 4]
            faddp st(1), st(0)
            fld dword ptr [edx]
            fmul dword ptr [edx]
            faddp st(1), st(0)
            fsqrt
            fdivr One
            fld dword ptr [edx]
            fmul st(0), st(1)
            fstp dword ptr [edx]
            fld dword ptr [edx + 4]
            fmul st(0), st(1)
            fstp dword ptr [edx + 4]
            fld dword ptr [edx + 8]
            fmul st(0), st(1)
            fstp dword ptr [edx + 8]
            fstp st(0)
        }
    }

    __declspec(noinline) void CrossLookAtVector(FVec3& Result, const FVec3& A, const FVec3& B, bool bReverseOrder)
    {
        // Retain the old matrix column before products. First cross computes
        // X/Y/Z; the second computes Z/Y/X. The ordinary Release candidate
        // changes operand stages and output before an unmasked exception.
        // A direct output reference also avoids Release NRVO losing the
        // local aggregate written through an asm LEA.
        FVec3* const ResultPtr = &Result;
        const FVec3* const APtr = &A;
        const FVec3* const BPtr = &B;
        __asm
        {
            mov edx, APtr
            mov ecx, BPtr
            mov eax, ResultPtr
            fld dword ptr [ecx]
            fld dword ptr [ecx + 4]
            fld dword ptr [ecx + 8]
            cmp bReverseOrder, 0
            jne LookAtReverseCross
            fld dword ptr [edx + 8]
            fmul st(0), st(2)
            fld dword ptr [edx + 4]
            fmul st(0), st(2)
            fsubp st(1), st(0)
            fstp dword ptr [eax]
            fld dword ptr [edx]
            fmul st(0), st(1)
            fld dword ptr [edx + 8]
            fmul st(0), st(4)
            fsubp st(1), st(0)
            fstp dword ptr [eax + 4]
            fstp st(0)
            fld dword ptr [edx + 4]
            fmul st(0), st(2)
            fld dword ptr [edx]
            fmul st(0), st(2)
            fsubp st(1), st(0)
            fstp dword ptr [eax + 8]
            fstp st(0)
            fstp st(0)
            jmp LookAtCrossDone
        LookAtReverseCross:
            fld dword ptr [edx]
            fmul st(0), st(2)
            fld dword ptr [edx + 4]
            fmul st(0), st(4)
            fsubp st(1), st(0)
            fstp dword ptr [eax + 8]
            fld dword ptr [edx + 8]
            fmul st(0), st(3)
            fld dword ptr [edx]
            fmul st(0), st(2)
            fsubp st(1), st(0)
            fstp dword ptr [eax + 4]
            fld dword ptr [edx + 4]
            fmul st(0), st(1)
            fld dword ptr [edx + 8]
            fmul st(0), st(3)
            fsubp st(1), st(0)
            fstp dword ptr [eax]
            fstp st(0)
            fstp st(0)
            fstp st(0)
        LookAtCrossDone:
        }
    }
    __declspec(noinline) void BuildLookAtFromTangent(const FVec3& Tangent, float* OutXform)
    {
        FVec3 Forward;
        std::memcpy(&Forward, &Tangent, sizeof(Forward));
        NormalizeLookAtVector(Forward);
        Forward.Z = -Forward.Z;
        Forward.Y = -Forward.Y;
        Forward.X = -Forward.X;

        FVec3 UpRef;
        std::memcpy(&UpRef.X, OutXform + 1, sizeof(float));
        std::memcpy(&UpRef.Y, OutXform + 4, sizeof(float));
        std::memcpy(&UpRef.Z, OutXform + 7, sizeof(float));

        FVec3 AxisYRaw;
        CrossLookAtVector(AxisYRaw, Forward, UpRef, false);
        FVec3 AxisY;
        std::memcpy(&AxisY, &AxisYRaw, sizeof(AxisY));
        NormalizeLookAtVector(AxisY);

        // 342f9..34333 reads the old X column, and uses the unnormalized
        // first cross retained in the other local vector.
        FVec3 ExistingX;
        std::memcpy(&ExistingX.X, OutXform, sizeof(float));
        std::memcpy(&ExistingX.Y, OutXform + 3, sizeof(float));
        std::memcpy(&ExistingX.Z, OutXform + 6, sizeof(float));
        FVec3 AxisZ;
        CrossLookAtVector(AxisZ, AxisYRaw, ExistingX, true);
        const float One = 1.0f;
        // 3435d..343d2: the Z product precedes the first caller store, its
        // float store follows it. Ordinary Release publishes output before
        // its unmasked invalid exception; retain this arithmetic/store span.
        const void* const Args[] = {OutXform, &AxisZ, &Forward, &AxisY, &One};
        __asm
        {
            lea esi, Args
            mov edx, dword ptr [esi + 4]
            mov edi, dword ptr [esi + 8]
            mov ebx, dword ptr [esi + 12]
            fld dword ptr [edx]
            fmul dword ptr [edx]
            fld dword ptr [edx + 4]
            fmul dword ptr [edx + 4]
            faddp st(1), st(0)
            fld dword ptr [edx + 8]
            fmul dword ptr [edx + 8]
            faddp st(1), st(0)
            fsqrt
            mov eax, dword ptr [esi + 16]
            fdivr dword ptr [eax]
            fld dword ptr [edx]
            fmul st(0), st(1)
            fstp dword ptr [edx]
            fld dword ptr [edx + 4]
            fmul st(0), st(1)
            fstp dword ptr [edx + 4]
            fld dword ptr [edx + 8]
            fmul st(0), st(1)
            mov ecx, dword ptr [esi]
            mov eax, dword ptr [edi + 0]
            mov dword ptr [ecx], eax
            fstp dword ptr [edx + 8]
            mov eax, dword ptr [ebx + 0]
            mov dword ptr [ecx + 4], eax
            fstp st(0)
            mov eax, dword ptr [edi + 4]
            mov dword ptr [ecx + 12], eax
            mov eax, dword ptr [edi + 8]
            mov dword ptr [ecx + 24], eax
            mov eax, dword ptr [ebx + 4]
            mov dword ptr [ecx + 16], eax
            mov eax, dword ptr [ebx + 8]
            mov dword ptr [ecx + 28], eax
            fld dword ptr [edx + 8]
            fchs
            fld dword ptr [edx + 4]
            fchs
            fld dword ptr [edx]
            fchs
            fstp dword ptr [edx]
            fstp dword ptr [edx + 4]
            fstp dword ptr [edx + 8]
            mov eax, dword ptr [edx + 4]
            mov dword ptr [ecx + 20], eax
            mov eax, dword ptr [edx]
            mov dword ptr [ecx + 8], eax
            mov eax, dword ptr [edx + 8]
            mov dword ptr [ecx + 32], eax
        }
    }
}
