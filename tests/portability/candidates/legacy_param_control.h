#pragma once
namespace LegacyParamControl
{
    __declspec(noinline) void ApplyHermiteTangents(FHermiteKnot* Knots, int Count, int TangentMode)
    {
        if (Count == 0 || Count == 1)
            return;

        for (int Index = 0; Index < Count - 1; ++Index)
        {
            FHermiteKnot* Current = &Knots[Index];
            FHermiteKnot* Next = &Knots[Index + 1];
            float DeltaT;
            float DeltaV;
            // 300e5..30110: both differences are stored before the clamp,
            // even for a mode whose branch does not calculate a slope.
            __asm
            {
                mov edx, Next
                mov ecx, Current
                fld dword ptr [edx + 4]
                fsub dword ptr [ecx + 4]
                fstp DeltaT
                fld dword ptr [edx]
                fsub dword ptr [ecx]
                fstp DeltaV
                fld DeltaT
                fcomp G_MinDeltaT
                fnstsw ax
                test ah, 05h
                jp TangentDispatch
                mov dword ptr DeltaT, 38d1b717h
            TangentDispatch:
            }
            switch (TangentMode)
            {
            case 0:
                __asm
                {
                    mov ecx, Current
                    mov dword ptr [ecx + 0ch], 0ffffffffh
                }
                break;
            case 1:
                __asm
                {
                    mov edx, Next
                    mov ecx, Current
                    fld DeltaV
                    fdiv DeltaT
                    fst dword ptr [edx + 8]
                    fstp dword ptr [ecx + 0ch]
                }
                break;
            case 4:
                __asm
                {
                    mov ecx, Current
                    mov edx, Next
                    mov dword ptr [ecx + 0ch], 0
                    mov dword ptr [edx + 8], 0
                }
                break;
            case 5:
                // TEST AH,41/JNZ takes not-greater, including unordered.
                // FNSTSW follows the half-product exactly as in retail.
                __asm
                {
                    mov ecx, Current
                    mov edx, Next
                    fld dword ptr [ecx]
                    fcomp dword ptr [edx]
                    fld DeltaT
                    fmul G_ScalarHalfFloat
                    fnstsw ax
                    fdivr DeltaV
                    test ah, 41h
                    jnz StoreOutgoing
                    mov dword ptr [ecx + 0ch], 0
                    fstp dword ptr [edx + 8]
                    jmp TangentDone
                StoreOutgoing:
                    fstp dword ptr [ecx + 0ch]
                    mov dword ptr [edx + 8], 0
                TangentDone:
                }
                break;
            case 2:
            case 3:
            case 6:
                // Mode6 TEST AH,05/JP selects mode3 on equal, greater or
                // unordered; the strictly-less fall-through selects mode2.
                __asm
                {
                    mov ecx, Current
                    mov edx, Next
                    cmp TangentMode, 2
                    je IncomingTangent
                    cmp TangentMode, 3
                    je OutgoingTangent
                    fld dword ptr [ecx]
                    fcomp dword ptr [edx]
                    fnstsw ax
                    test ah, 05h
                    jp OutgoingTangent
                IncomingTangent:
                    fld DeltaT
                    mov dword ptr [ecx + 0ch], 0
                    fmul G_ScalarHalfFloat
                    fdivr DeltaV
                    fstp dword ptr [edx + 8]
                    jmp TangentPairDone
                OutgoingTangent:
                    fld DeltaT
                    fmul G_ScalarHalfFloat
                    fdivr DeltaV
                    fstp dword ptr [ecx + 0ch]
                    mov dword ptr [edx + 8], 0
                TangentPairDone:
                }
                break;
            default:
                break;
            }
        }
    }

__declspec(noinline) float WrappedRatio(int A,int B)
{
    int Remainder;float Normalized;
            __asm
            {
                mov eax, A
                cdq
                idiv B
                mov Remainder, edx
                fild Remainder
                fidiv B
                fstp dword ptr Normalized
            }
    return Normalized;
}

__declspec(noinline) bool ReplacementTime(int Id,float F0,float* output)
{
            float TargetT;
            unsigned char bAccepted;
            const float Zero = 0.0f;
            const float One = 1.0f;
            const double EndpointZero = 0.0;
            const double EndpointOne = 1.0;
            // Match the parity branches, including unordered. TEST AH,05/JP
            // retains unordered; TEST AH,44/JP bypasses the endpoint-ID gate
            // on unequal OR unordered. A NaN time is not rejected here.
            __asm
            {
                mov ecx, Id
                test ecx, ecx
                jz ReplaceTimeZero
                cmp ecx, 1
                jne ReplaceClampTime
                fld One
                jmp ReplaceEndpointZero
            ReplaceClampTime:
                fld F0
                fcom One
                fnstsw ax
                test ah, 41h
                jnz ReplaceLowerBound
                fstp st(0)
                fld One
                jmp ReplaceEndpointZero
            ReplaceLowerBound:
                fcom Zero
                fnstsw ax
                test ah, 05h
                jp ReplaceEndpointZero
                fstp st(0)
            ReplaceTimeZero:
                fld Zero
            ReplaceEndpointZero:
                fcom EndpointZero
                fnstsw ax
                test ah, 44h
                jp ReplaceEndpointOne
                test ecx, ecx
                jnz ReplaceRejected
            ReplaceEndpointOne:
                fcom EndpointOne
                fnstsw ax
                test ah, 44h
                jp ReplaceAccepted
                cmp ecx, 1
                jne ReplaceRejected
            ReplaceAccepted:
                fstp TargetT
                mov bAccepted, 1
                jmp ReplaceTimeDone
            ReplaceRejected:
                fstp st(0)
                mov bAccepted, 0
            ReplaceTimeDone:
            }
    if (bAccepted) std::memcpy(output,&TargetT,4);
    return bAccepted!=0;
}

__declspec(noinline) int ClassifyTime(float F0)
{
            const float Zero = 0.0f;
            const float One = 1.0f;
            int TimeClass;
            // 2f560..2f5f0: reject strictly outside, select only ordered
            // endpoint equality; unordered reaches new-key construction.
            __asm
            {
                fld F0
                fcomp Zero
                fnstsw ax
                test ah, 05h
                jnp AddTimeRejected
                fld F0
                fcomp One
                fnstsw ax
                test ah, 41h
                jz AddTimeRejected
                fld F0
                fcomp Zero
                fnstsw ax
                test ah, 44h
                jnp AddTimeZero
                fld F0
                fcomp One
                fnstsw ax
                test ah, 44h
                jnp AddTimeOne
                mov TimeClass, 3
                jmp AddTimeDone
            AddTimeRejected:
                mov TimeClass, -1
                jmp AddTimeDone
            AddTimeZero:
                mov TimeClass, 0
                jmp AddTimeDone
            AddTimeOne:
                mov TimeClass, 1
            AddTimeDone:
            }
    return TimeClass;
}

__declspec(noinline) float NormalizeTime(const float* T,const float* T0,const float* T1)
{
    float U;
            __asm
            {
                mov eax, T0
                mov ecx, T1
                mov edx, T
                fld dword ptr [edx]
                fsub dword ptr [eax]
                fld dword ptr [ecx]
                fsub dword ptr [eax]
                fdivp st(1), st(0)
                fstp U
            }
    return U;
}
}
