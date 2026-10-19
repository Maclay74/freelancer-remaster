#pragma once
namespace LegacyCurvePolynomial
{
    __declspec(naked) void __cdecl CatmullRom1D(const float* P0, const float* P1,
                                               const float* P2, const float* P3, float U, FThornExtendedProgress* Output)
    {
        __asm
        {
            fld dword ptr [esp + 14h]
            mov eax, dword ptr [esp + 4]
            test eax, eax
            fmul dword ptr [esp + 14h]
            fld dword ptr [esp + 14h]
            push esi
            mov esi, dword ptr [esp + 0ch]
            fmul st(0), st(1)
            jnz ScalarLeftPresent
            mov eax, esi
        ScalarLeftPresent:
            mov edx, dword ptr [esp + 14h]
            test edx, edx
            mov ecx, dword ptr [esp + 10h]
            jnz ScalarRightPresent
            mov edx, ecx
        ScalarRightPresent:
            fld dword ptr [eax]
            fadd st(0), st(0)
            fld dword ptr [esi]
            fmul G_ScalarFive
            fsubp st(1), st(0)
            fld dword ptr [ecx]
            fmul G_ScalarFour
            faddp st(1), st(0)
            fsub dword ptr [edx]
            fmulp st(2), st(0)
            fld dword ptr [esi]
            fmul G_ScalarThree
            fsub dword ptr [eax]
            fld dword ptr [ecx]
            fmul G_ScalarThree
            fsubp st(1), st(0)
            fadd dword ptr [edx]
            fmul st(0), st(1)
            faddp st(2), st(0)
            fld dword ptr [ecx]
            fsub dword ptr [eax]
            fmul dword ptr [esp + 18h]
            faddp st(2), st(0)
            fld dword ptr [esi]
            pop esi
            fadd st(0), st(0)
            faddp st(2), st(0)
            fxch st(1)
            fmul G_ScalarHalf
            fstp st(1)
            mov eax, dword ptr [esp + 18h]
            fstp tbyte ptr [eax]
            ret
        }
    }

    __declspec(naked) void __cdecl ParamLerp(const FParamKey* Left, const FParamKey* Right, const float* T, FThornExtendedProgress* Output)
    {
        // 2f948..2f961: extended subtraction/divide/product/add to 80-bit output.
        __asm
        {
            // Copy input bits into private argument scratch, without an FP load.
            mov eax, dword ptr [esp + 0ch]
            mov eax, dword ptr [eax]
            mov dword ptr [esp + 0ch], eax
            mov eax, dword ptr [esp + 4]
            mov ecx, dword ptr [esp + 8]
            fld dword ptr [eax + 4]
            fld dword ptr [esp + 0ch]
            fsub dword ptr [eax]
            fld dword ptr [ecx]
            fsub dword ptr [eax]
            fdivp st(1), st(0)
            fld dword ptr [ecx + 4]
            fsub st(0), st(2)
            fmulp st(1), st(0)
            faddp st(1), st(0)
            mov eax, dword ptr [esp + 10h]
            fstp tbyte ptr [eax]
            ret
        }
    }

    __declspec(naked) void __cdecl Hermite1D(const FHermiteKnot& A, const FHermiteKnot& B, const float* T, FThornExtendedProgress* Output)
    {
        __asm
        {
            // Copy input bits into private argument scratch, without an FP load.
            mov eax, dword ptr [esp + 0ch]
            mov eax, dword ptr [eax]
            mov dword ptr [esp + 0ch], eax
            mov ecx, dword ptr [esp + 4]
            fld dword ptr [ecx + 4]
            fcomp dword ptr [esp + 0ch]
            fnstsw ax
            test ah, 44h
            jp CompareRight
            fld dword ptr [ecx]
            jmp HermiteStoreOutput
        CompareRight:
            mov edx, dword ptr [esp + 8]
            fld dword ptr [edx + 4]
            fcomp dword ptr [esp + 0ch]
            fnstsw ax
            test ah, 44h
            jp NormalizeTime
            fld dword ptr [edx]
            jmp HermiteStoreOutput
        NormalizeTime:
            fld dword ptr [edx + 4]
            mov eax, dword ptr [ecx + 0ch]
            cmp eax, -1
            fsub dword ptr [ecx + 4]
            fld dword ptr [esp + 0ch]
            fsub dword ptr [ecx + 4]
            fdiv st(0), st(1)
            fstp dword ptr [esp + 0ch]
            jnz EvaluatePolynomial
            fstp st(0)
            fld dword ptr [ecx]
            jmp HermiteStoreOutput
        EvaluatePolynomial:
            fld dword ptr [esp + 0ch]
            fmul dword ptr [esp + 0ch]
            fld dword ptr [esp + 0ch]
            fmul st(0), st(1)
            fld st(0)
            fadd st(0), st(1)
            fld st(2)
            fmul G_ScalarThree
            fld st(3)
            fadd st(0), st(0)
            fsubr st(0), st(3)
            fadd dword ptr [esp + 0ch]
            fmul dword ptr [ecx + 0ch]
            fmul st(0), st(5)
            fxch st(3)
            fsub st(0), st(4)
            fmul dword ptr [edx + 8]
            fmul st(0), st(5)
            faddp st(3), st(0)
            fld st(1)
            fsub st(0), st(1)
            fadd G_ScalarOne
            fmul dword ptr [ecx]
            faddp st(3), st(0)
            fsub st(0), st(1)
            fmul dword ptr [edx]
            faddp st(2), st(0)
            fxch st(1)
            fstp st(3)
            fstp st(0)
            fstp st(0)
        HermiteStoreOutput:
            mov eax, dword ptr [esp + 10h]
            fstp tbyte ptr [eax]
            ret
        }
    }

    __declspec(noinline) void HermiteEvaluate(const FHermiteKnot* Begin, int Count, const float* T, FThornExtendedProgress* Output)
    {
        // A named local operand can use the compiler's realigned frame register.
        // Clang may choose ESI for that register, which this asm also modifies.
        // Address all operands through one fixed EBX state pointer instead.
        struct FState
        {
            const FHermiteKnot* Begin;
            int Count;
            const float* T;
            FThornExtendedProgress* Output;
            const FHermiteKnot* Left;
            const FHermiteKnot* Right;
            int LastIndex;
            float FirstTime;
            float LastTime;
            unsigned short OldControl;
            unsigned short ChopControl;
            // FISTP still writes eight bytes, without imposing 8-byte C++
            // alignment and introducing another realigned frame register.
            unsigned int Seed[2];
            unsigned char bEndpoint;
        } State = {Begin, Count, T, Output};
        static_assert(offsetof(FState, T) == 8 && offsetof(FState, Left) == 0x10 &&
                      offsetof(FState, OldControl) == 0x24 && offsetof(FState, Seed) == 0x28 &&
                      offsetof(FState, bEndpoint) == 0x30 && alignof(FState) == 4, "Hermite asm state offsets");
        __asm
        {
            lea ebx, State
            mov esi, dword ptr [ebx]
            test esi, esi
            jnz HermiteReadCount
            xor eax, eax
            jmp HermiteCountReady
        HermiteReadCount:
            mov eax, dword ptr [ebx + 4]
        HermiteCountReady:
            cmp eax, 1
            jne HermiteCheckEndpoints
            fld dword ptr [esi]
            jmp HermiteEndpointOutput
        HermiteCheckEndpoints:
            mov edx, dword ptr [ebx + 8]
            fld dword ptr [edx]
            dec eax
            mov dword ptr [ebx + 18h], eax
            shl eax, 4
            add eax, esi
            mov edi, eax
            mov eax, dword ptr [esi + 4]
            mov ecx, dword ptr [edi + 4]
            mov dword ptr [ebx + 1ch], eax
            fcomp dword ptr [ebx + 1ch]
            mov dword ptr [ebx + 20h], ecx
            fnstsw ax
            test ah, 05h
            jp HermiteCheckLastEndpoint
            fld dword ptr [esi]
            jmp HermiteEndpointOutput
        HermiteCheckLastEndpoint:
            mov edx, dword ptr [ebx + 8]
            fld dword ptr [edx]
            fcomp dword ptr [ebx + 20h]
            fnstsw ax
            test ah, 41h
            jnz HermiteSeedSegment
            fld dword ptr [edi]
            jmp HermiteEndpointOutput
        HermiteSeedSegment:
            mov edx, dword ptr [ebx + 8]
            fld dword ptr [edx]
            fsub dword ptr [ebx + 1ch]
            fld dword ptr [ebx + 20h]
            fsub dword ptr [ebx + 1ch]
            fdivp st(1), st(0)
            fimul dword ptr [ebx + 18h]
            fstcw word ptr [ebx + 24h]
            fwait
            mov ax, word ptr [ebx + 24h]
            or ah, 0ch
            mov word ptr [ebx + 26h], ax
            fldcw word ptr [ebx + 26h]
            fistp qword ptr [ebx + 28h]
            fldcw word ptr [ebx + 24h]
            mov eax, dword ptr [ebx + 28h]
            mov ecx, eax
            shl ecx, 4
            fld dword ptr [ecx + esi + 4]
            add ecx, esi
            mov edx, dword ptr [ebx + 8]
            fcomp dword ptr [edx]
            fnstsw ax
            test ah, 41h
            jnz HermiteWalkForwardStart
            cmp ecx, esi
            jbe HermiteBackwardPair
        HermiteWalkBackward:
            fld dword ptr [ecx + 4]
            mov edx, dword ptr [ebx + 8]
            fcomp dword ptr [edx]
            fnstsw ax
            test ah, 05h
            jnp HermiteBackwardPair
            sub ecx, 10h
            cmp ecx, esi
            ja HermiteWalkBackward
        HermiteBackwardPair:
            lea eax, [ecx + 10h]
            jmp HermiteSelectedPair
        HermiteWalkForwardStart:
            cmp ecx, edi
            ja HermiteForwardPair
        HermiteWalkForward:
            fld dword ptr [ecx + 4]
            mov edx, dword ptr [ebx + 8]
            fcomp dword ptr [edx]
            fnstsw ax
            test ah, 41h
            jz HermiteForwardPair
            add ecx, 10h
            cmp ecx, edi
            jbe HermiteWalkForward
        HermiteForwardPair:
            mov eax, ecx
            add ecx, -10h
        HermiteSelectedPair:
            mov dword ptr [ebx + 10h], ecx
            mov dword ptr [ebx + 14h], eax
            jmp HermitePhaseDone
        HermiteEndpointOutput:
            mov ecx, dword ptr [ebx + 0ch]
            fstp tbyte ptr [ecx]
            mov byte ptr [ebx + 30h], 1
        HermitePhaseDone:
        }
        if (!State.bEndpoint)
            Hermite1D(*State.Left, *State.Right, T, Output);
    }
}
