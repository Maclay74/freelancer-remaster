#pragma once

namespace LegacySplineQuaternion
{
static bool IsQuatLogAccepted(const float& w) { return std::fabs(static_cast<double>(w))<1.0; }
    __declspec(noinline) void QuatInverseScaleRetail(const FQuat& Q, float& Scale)
    {
        const float One = 1.0f;
        const FQuat* const QPtr = &Q;
        float* const ScalePtr = &Scale;
        __asm
        {
            mov ecx, ScalePtr
            mov edx, QPtr
            fld dword ptr [edx + 4]
            fmul st(0), st(0)
            fld dword ptr [edx + 8]
            fmul st(0), st(0)
            faddp st(1), st(0)
            fld dword ptr [edx + 12]
            fmul st(0), st(0)
            faddp st(1), st(0)
            fld dword ptr [edx]
            fmul st(0), st(0)
            faddp st(1), st(0)
            fdivr One
            fstp dword ptr [ecx]
        }
    }

    __declspec(noinline) float SquadBlendRetail(const float* T)
    {
        const float One = 1.0f;
        float Blend;
        __asm
        {
            mov ecx, T
            fld One
            fsub dword ptr [ecx]
            fmul dword ptr [ecx]
            fadd st(0), st(0)
            fstp Blend
        }
        return Blend;
    }

    __declspec(noinline) void QuatAdd(FQuat& Dst, const FQuat& A, const FQuat& B)
    {
        // Ordinary double temporaries spill to qword in Debug. At PC64,
        // 1 + 2^-60 then traps on that spill before the retail W publication.
        FQuat* const DstPtr = &Dst;
        const FQuat* const APtr = &A;
        const FQuat* const BPtr = &B;
        __asm
        {
            mov edx, APtr
            mov ecx, BPtr
            fld dword ptr [edx + 12]
            fadd dword ptr [ecx + 12]
            fld dword ptr [edx + 8]
            fadd dword ptr [ecx + 8]
            fld dword ptr [edx + 4]
            fadd dword ptr [ecx + 4]
            fld dword ptr [edx]
            fadd dword ptr [ecx]
            mov edx, DstPtr
            fstp dword ptr [edx]
            fstp dword ptr [edx + 4]
            fstp dword ptr [edx + 8]
            fstp dword ptr [edx + 12]
        }
    }

    __declspec(noinline) void QuatScale(FQuat& Dst, const FQuat& Q, float S)
    {
        float W;
        float X;
        float Y;
        float Z;
        // Release C++ retained S and reversed the first three FMUL operands:
        // the differential probe found NaN payload and denormal flag changes.
        // Explicit pointer operands avoid compiler-specific reference dereferencing.
        auto* const DstAsmPtr = &Dst;
        auto* const QAsmPtr = &Q;
        __asm
        {
            mov edx, QAsmPtr
            fld S
            fmul dword ptr [edx]
            fstp W
            fld S
            fmul dword ptr [edx + 4]
            fstp X
            fld S
            fmul dword ptr [edx + 8]
            fstp Y
            fld S
            fmul dword ptr [edx + 12]
            mov edx, DstAsmPtr
            mov eax, W
            mov dword ptr [edx], eax
            mov eax, X
            mov dword ptr [edx + 4], eax
            fstp Z
            mov eax, Y
            mov dword ptr [edx + 8], eax
            mov eax, Z
            mov dword ptr [edx + 12], eax
        }
    }

    __declspec(noinline) void QuatLogRetail(FQuat& Dst, const FQuat& Q, bool bRoundScale)
    {
        if (!IsQuatLogAccepted(Q.W))
        {
            const unsigned int Zero = 0;
            std::memcpy(&Dst.W, &Zero, sizeof(Zero));
            std::memcpy(&Dst.X, &Q.X, sizeof(float));
            std::memcpy(&Dst.Y, &Q.Y, sizeof(float));
            std::memcpy(&Dst.Z, &Q.Z, sizeof(float));
            return;
        }

        // The mandatory modern std::acos call publishes a double result.
        // That published double is the input to the owned arithmetic below.
        const double Angle = std::acos(static_cast<double>(Q.W));
        const float Epsilon = G_QuatSinEps;
        float Scale;
        float X;
        float Y;
        float Z;
        bool bFallback = false;
        // The intrinsic candidate spills Z before publishing X/Y: with a
        // denormal Z and unmasked DE, both builds trap before those stores.
        // 241ce..241f0 publishes X/Y between the Z product and its float store.
        // Explicit pointer operands avoid compiler-specific reference dereferencing.
        auto* const DstAsmPtr = &Dst;
        auto* const QAsmPtr = &Q;
        __asm
        {
            fld Angle
            fld st(0)
            fsin
            fcom Epsilon
            fnstsw ax
            test ah, 1
            jnz LogCopyInput
            fxch st(1)
            fdiv st(0), st(1)
            cmp bRoundScale, 0
            je LogRetainedProducts
            // 218e9 stores Scale before discarding the retained sine.
            fstp Scale
            fstp st(0)
            jmp LogArithmeticDone
        LogRetainedProducts:
            fxch st(1)
            fstp st(0)
            mov edx, QAsmPtr
            fld st(0)
            fmul dword ptr [edx + 4]
            fstp X
            fld st(0)
            fmul dword ptr [edx + 8]
            fstp Y
            fmul dword ptr [edx + 12]
            mov ecx, DstAsmPtr
            mov eax, X
            mov dword ptr [ecx + 4], eax
            mov eax, Y
            mov dword ptr [ecx + 8], eax
            fstp Z
            mov dword ptr [ecx], 0
            mov eax, Z
            mov dword ptr [ecx + 12], eax
            jmp LogArithmeticDone
        LogCopyInput:
            fstp st(0)
            fstp st(0)
            mov bFallback, 1
        LogArithmeticDone:
        }
        if (bFallback)
        {
            const unsigned int Zero = 0;
            std::memcpy(&Dst.W, &Zero, sizeof(Zero));
            std::memcpy(&Dst.X, &Q.X, sizeof(float));
            std::memcpy(&Dst.Y, &Q.Y, sizeof(float));
            std::memcpy(&Dst.Z, &Q.Z, sizeof(float));
            return;
        }
        if (bRoundScale)
        {
            FVec3 Scaled;
            ThornSplineMath::Scale(&Scaled.X, &Q.X, Scale);
            const unsigned int Zero = 0;
            std::memcpy(&Dst.W, &Zero, sizeof(Zero));
            std::memcpy(&Dst.X, &Scaled.X, sizeof(float));
            std::memcpy(&Dst.Y, &Scaled.Y, sizeof(float));
            std::memcpy(&Dst.Z, &Scaled.Z, sizeof(float));
        }
    }

    __declspec(noinline) void QuatFirstControlExp(FQuat& Dst, const FQuat& Q)
    {
        const float Epsilon = G_QuatSinEps;
        float Norm;
        float Scale;
        bool bScaleVector = false;
        // Intrinsic C++ changes FSIN/FCOS results and drops the float scale
        // boundary in Release; the root PC/RC probe confirms numeric cases.
        // Explicit pointer operands avoid compiler-specific reference dereferencing.
        auto* const QAsmPtr = &Q;
        __asm
        {
            mov edx, QAsmPtr
            fld dword ptr [edx + 4]
            fmul dword ptr [edx + 4]
            fld dword ptr [edx + 8]
            fmul dword ptr [edx + 8]
            faddp st(1), st(0)
            fld dword ptr [edx + 12]
            fmul dword ptr [edx + 12]
            faddp st(1), st(0)
            fsqrt
            fst Norm
            fsin
            fcom Epsilon
            fnstsw ax
            test ah, 1
            jnz FirstExpCopyVector
            fdiv Norm
            fstp Scale
            mov bScaleVector, 1
            jmp FirstExpVectorReady
        FirstExpCopyVector:
            fstp st(0)
        FirstExpVectorReady:
        }
        FQuat Result;
        if (bScaleVector)
            ThornSplineMath::Scale(&Result.X, &Q.X, Scale);
        else
        {
            std::memcpy(&Result.X, &Q.X, sizeof(float));
            std::memcpy(&Result.Y, &Q.Y, sizeof(float));
            std::memcpy(&Result.Z, &Q.Z, sizeof(float));
        }
        // FCOS follows Scale3D, and consumes its explicit float norm.
        __asm
        {
            fld Norm
            fcos
            fstp Result.W
        }
        std::memcpy(&Dst, &Result, sizeof(Result));
    }

    __declspec(noinline) void QuatExpRetail(FQuat& Dst, const FQuat& Q, bool bKeepNormForSin)
    {
        const float Epsilon = G_QuatSinEps;
        float Norm;
        FQuat Result;
        Result.X = Q.X;
        Result.Y = Q.Y;
        Result.Z = Q.Z;
        // Explicit pointer operands avoid compiler-specific reference dereferencing.
        auto* const QAsmPtr = &Q;
        __asm
        {
            mov edx, QAsmPtr
            fld dword ptr [edx + 4]
            fmul st(0), st(0)
            fld dword ptr [edx + 8]
            fmul st(0), st(0)
            faddp st(1), st(0)
            fld dword ptr [edx + 12]
            fmul st(0), st(0)
            faddp st(1), st(0)
            fsqrt
            fst Norm
            cmp bKeepNormForSin, 0
            jne ExpSin
            fstp st(0)
            fld Norm
        ExpSin:
            fsin
            fcom Epsilon
            fnstsw ax
            test ah, 01h
            jnz ExpPopSin
            fdiv Norm
            lea ecx, Result
            fld st(0)
            fmul dword ptr [edx + 4]
            fstp dword ptr [ecx + 4]
            fld st(0)
            fmul dword ptr [edx + 8]
            fstp dword ptr [ecx + 8]
            fmul dword ptr [edx + 12]
            fstp dword ptr [ecx + 12]
            jmp ExpCos
        ExpPopSin:
            fstp st(0)
            // Standalone fallback copies via FLD/FSTP; the inline first
            // control copies raw words. Preserve even signaling NaN handling.
            cmp bKeepNormForSin, 0
            jne ExpCos
            lea ecx, Result
            fld dword ptr [edx + 12]
            fld dword ptr [edx + 8]
            fld dword ptr [edx + 4]
            fld Norm
            fcos
            fstp dword ptr [ecx]
            fstp dword ptr [ecx + 4]
            fstp dword ptr [ecx + 8]
            fstp dword ptr [ecx + 12]
            jmp ExpDone
        ExpCos:
            lea ecx, Result
            fld Norm
            fcos
            fstp dword ptr [ecx]
        ExpDone:
        }
        Dst = Result;
    }
}
