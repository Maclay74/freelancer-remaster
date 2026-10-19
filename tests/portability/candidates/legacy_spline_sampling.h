#pragma once
namespace LegacySplineSampling
{
    __declspec(noinline) void BuildSlot5MatrixRetail(const FVec3& Tangent, float* Mat9)
    {
        float Scratch[27];
        std::memcpy(Scratch + 3, &Tangent, sizeof(Tangent));
        const float One = 1.0f;
        const float Zero = 0.0f;
        __asm
        {
            lea edx, Scratch
            fld dword ptr [edx + 14h]
            fmul dword ptr [edx + 14h]
            fld dword ptr [edx + 10h]
            fmul dword ptr [edx + 10h]
            faddp st(1), st(0)
            fld dword ptr [edx + 0ch]
            fmul dword ptr [edx + 0ch]
            faddp st(1), st(0)
            fsqrt
            fdivr One
            fld dword ptr [edx + 0ch]
            fmul st(0), st(1)
            fstp dword ptr [edx + 0ch]
            fld dword ptr [edx + 10h]
            fmul st(0), st(1)
            fstp dword ptr [edx + 10h]
            fld dword ptr [edx + 14h]
            fmul st(0), st(1)
            fstp dword ptr [edx + 14h]
            fstp st(0)
            fld dword ptr [edx + 0ch]
            fchs
            fstp dword ptr [edx + 0ch]
            fld dword ptr [edx + 10h]
            fchs
            fstp dword ptr [edx + 10h]
            fld dword ptr [edx + 14h]
            fchs
            fstp dword ptr [edx + 14h]
            fld dword ptr [edx + 10h]
            fmul Zero
            fld dword ptr [edx + 14h]
            fsub st(0), st(1)
            fstp dword ptr [edx + 18h]
            fld dword ptr [edx + 0ch]
            fmul Zero
            fld dword ptr [edx + 14h]
            fmul Zero
            fsubp st(1), st(0)
            fstp dword ptr [edx + 1ch]
            fsub dword ptr [edx + 0ch]
            fst dword ptr [edx + 20h]
            fmul dword ptr [edx + 20h]
            fld dword ptr [edx + 1ch]
            fmul dword ptr [edx + 1ch]
            faddp st(1), st(0)
            fld dword ptr [edx + 18h]
            fmul dword ptr [edx + 18h]
            faddp st(1), st(0)
            fsqrt
            fdivr One
            fstp dword ptr [edx + 68h]
            fld dword ptr [edx + 18h]
            fmul dword ptr [edx + 68h]
            fld dword ptr [edx + 1ch]
            fmul dword ptr [edx + 68h]
            fld dword ptr [edx + 20h]
            fmul dword ptr [edx + 68h]
            fstp dword ptr [edx + 20h]
            fld dword ptr [edx + 10h]
            fmul dword ptr [edx + 20h]
            fld dword ptr [edx + 14h]
            fmul st(0), st(2)
            fsubp st(1), st(0)
            fld dword ptr [edx + 14h]
            fmul st(0), st(3)
            fld dword ptr [edx + 0ch]
            fmul dword ptr [edx + 20h]
            fsubp st(1), st(0)
            fstp dword ptr [edx + 28h]
            fld dword ptr [edx + 0ch]
            fmul st(0), st(2)
            fld dword ptr [edx + 10h]
            fmul st(0), st(4)
            fsubp st(1), st(0)
            fst dword ptr [edx + 2ch]
            fmul dword ptr [edx + 2ch]
            fld dword ptr [edx + 28h]
            fmul dword ptr [edx + 28h]
            faddp st(1), st(0)
            fld st(1)
            fmul st(0), st(2)
            faddp st(1), st(0)
            fsqrt
            fdivr One
            fstp dword ptr [edx + 68h]
            fmul dword ptr [edx + 68h]
            fld dword ptr [edx + 28h]
            fmul dword ptr [edx + 68h]
            fld dword ptr [edx + 2ch]
            fmul dword ptr [edx + 68h]
            fstp dword ptr [edx + 5ch]
            fxch st(1)
            fstp dword ptr [edx + 44h]
            fstp dword ptr [edx + 50h]
            fxch st(1)
            fstp dword ptr [edx + 40h]
            fstp dword ptr [edx + 4ch]
        }
        std::memcpy(Scratch + 18, Scratch + 3, sizeof(float));
        std::memcpy(Scratch + 21, Scratch + 4, sizeof(float));
        std::memcpy(Scratch + 24, Scratch + 5, sizeof(float));
        std::memcpy(Scratch + 22, Scratch + 8, sizeof(float));
        std::memcpy(Mat9, Scratch + 16, 9 * sizeof(float));
    }

    __declspec(noinline) void AccumulateSegmentSampleRetail(FVec3& Prev, const FVec3& Cur,
                                       const float& GlobalOffset, float& SegmentLength,
                                       float& SampleTime)
    {
        // One addressable argument block avoids exhausting clang-cl's inline
        // asm operand registers; the x87 instruction sequence is unchanged.
        const void* const Args[] = {&Prev, &Cur, &GlobalOffset, &SegmentLength, &SampleTime};
        __asm
        {
            lea esi, Args
            mov edx, dword ptr [esi + 4]
            mov ecx, dword ptr [esi]
            fld dword ptr [edx]
            fsub dword ptr [ecx]
            mov eax, dword ptr [edx]
            fld dword ptr [edx + 4]
            mov ebx, dword ptr [edx + 8]
            fsub dword ptr [ecx + 4]
            mov edi, dword ptr [edx + 4]
            fld dword ptr [edx + 8]
            mov dword ptr [ecx], eax
            fsub dword ptr [ecx + 8]
            fld st(0)
            mov dword ptr [ecx + 8], ebx
            fmul st(0), st(1)
            fld st(2)
            fmul st(0), st(3)
            mov dword ptr [ecx + 4], edi
            faddp st(1), st(0)
            fld st(3)
            fmul st(0), st(4)
            faddp st(1), st(0)
            fsqrt
            fstp st(3)
            fstp st(0)
            fstp st(0)
            mov eax, dword ptr [esi + 12]
            fadd dword ptr [eax]
            fst dword ptr [eax]
            mov eax, dword ptr [esi + 8]
            fadd dword ptr [eax]
            mov edx, dword ptr [esi + 16]
            fstp dword ptr [edx]
        }
    }

    __declspec(noinline) void NormalizeSegmentSamplesRetail(float* Table, float Total)
    {
        const float One = 1.0f;
        const int Count = G_SamplesPerSegment;
        __asm
        {
            mov edx, Table
            mov ecx, Count
            fld One
            fdiv Total
        NormalizeSample:
            fld st(0)
            fmul dword ptr [edx]
            fstp dword ptr [edx]
            add edx, 4
            dec ecx
            jnz NormalizeSample
            fstp st(0)
        }
    }

        __declspec(noinline) void SubtractGhostVector(FVec3& OutputVector, const FVec3& A, const FVec3& B)
        {
            // Explicit pointer operands avoid compiler-specific reference dereferencing.
            auto* const OutputVectorAsmPtr = &OutputVector;
            auto* const AAsmPtr = &A;
            auto* const BAsmPtr = &B;
            __asm
            {
                mov eax, AAsmPtr
                mov ecx, BAsmPtr
                mov edx, OutputVectorAsmPtr
                fld dword ptr [eax + 8]
                fsub dword ptr [ecx + 8]
                fld dword ptr [eax + 4]
                fsub dword ptr [ecx + 4]
                fld dword ptr [eax]
                fsub dword ptr [ecx]
                fstp dword ptr [edx]
                fstp dword ptr [edx + 4]
                fstp dword ptr [edx + 8]
            }
        }

__declspec(noinline) void ExtendEndGhost(const FVec3& last,FVec3& ghost)
{
    const FVec3* LastPoint=&last;
    FVec3* GhostPoint=&ghost;
            float DifferenceZ;
            float X;
            float Y;
            float Z;
            // Release reassociates the ordinary C++ additions and selects
            // different NaN payloads (58 concrete probe mismatches).
            // Keep only this arithmetic/publication sequence bounded here.
            __asm
            {
                mov ecx, LastPoint
                mov edx, GhostPoint
                fld dword ptr [ecx]
                fsub dword ptr [edx]
                fld dword ptr [ecx + 4]
                fsub dword ptr [edx + 4]
                fld dword ptr [ecx + 8]
                fsub dword ptr [edx + 8]
                fstp DifferenceZ
                fxch st(1)
                fadd dword ptr [ecx]
                fstp X
                fadd dword ptr [ecx + 4]
                fstp Y
                fld DifferenceZ
                fadd dword ptr [ecx + 8]
                mov eax, X
                mov dword ptr [edx], eax
                mov eax, Y
                mov dword ptr [edx + 4], eax
                fstp Z
                mov eax, Z
                mov dword ptr [edx + 8], eax
            }
}
}
