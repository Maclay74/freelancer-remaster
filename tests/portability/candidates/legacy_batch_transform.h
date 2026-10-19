// Original specialized batch-transform path. Test oracle only.
#pragma once
#include <cstddef>
#include "legacy_fast_dot_helpers.h"
namespace LegacyBatchMath
{
    // FastDot (FUN_06f71af0 + LAB_06f71d60…LAB_06f71f90): classify each row coeff
    // as ==0 / ==1 / other. Retail layout: kernel @ +0xC, saved-ops int @ +0x10.
    // Callers: FUN_06f719b0 (slot 17) and LAB_06f71fb0 (slot 16) only — both wired.
    // Kernels are isolated thiscall / RET 4 / ST0. Slots 16/17 stay DACOM stdcall.
    struct FFastDot
    {
        float A;
        float B;
        float C;
#if defined(__clang__) && defined(_M_IX86)
        // Clang rejects naked member functions. A fastcall entry keeps this in
        // ECX and Vec on the stack (unused EDX), preserving the RET 4/ST0 ABI.
        float (__fastcall *Kernel)(FFastDot*, void*, const FVector*);
#else
        float (FFastDot::*Kernel)(const FVector* Vec);
#endif
        int SavedOps;

        float FastDotZZZ(const FVector* Vec);
        float FastDotZZO(const FVector* Vec);
        float FastDotZZG(const FVector* Vec);
        float FastDotZOZ(const FVector* Vec);
        float FastDotZOO(const FVector* Vec);
        float FastDotZOG(const FVector* Vec);
        float FastDotZGZ(const FVector* Vec);
        float FastDotZGO(const FVector* Vec);
        float FastDotZGG(const FVector* Vec);
        float FastDotOZZ(const FVector* Vec);
        float FastDotOZO(const FVector* Vec);
        float FastDotOZG(const FVector* Vec);
        float FastDotOOZ(const FVector* Vec);
        float FastDotOOO(const FVector* Vec);
        float FastDotOOG(const FVector* Vec);
        float FastDotOGZ(const FVector* Vec);
        float FastDotOGO(const FVector* Vec);
        float FastDotOGG(const FVector* Vec);
        float FastDotGZZ(const FVector* Vec);
        float FastDotGZO(const FVector* Vec);
        float FastDotGZG(const FVector* Vec);
        float FastDotGOZ(const FVector* Vec);
        float FastDotGOO(const FVector* Vec);
        float FastDotGOG(const FVector* Vec);
        float FastDotGGZ(const FVector* Vec);
        float FastDotGGO(const FVector* Vec);
        float FastDotGGG(const FVector* Vec);
    };

#if defined(__clang__) && defined(_M_IX86)
    typedef float (__fastcall *FFastDotKernel)(FFastDot*, void*, const FVector*);
#define FAST_DOT_ENTRY(Name) __fastcall Name(FFastDot*, void*, const FVector* Vec)
#define FAST_DOT_ADDRESS(Name) &Name
#else
    typedef float (FFastDot::*FFastDotKernel)(const FVector* Vec);
#define FAST_DOT_ENTRY(Name) FFastDot::Name(const FVector* Vec)
#define FAST_DOT_ADDRESS(Name) &FFastDot::Name
#endif

    int ClassifyLane(float X)
    {
        if (X == 0.0f)
            return 0;
        if (X == 1.0f)
            return 1;
        return 2;
    }

#if defined(_M_IX86)
    // DAT_06f730fc — FLD of +0.0; FLDZ is the same value (ZZZ only).
    float FAST_DOT_ENTRY(FastDotZZZ)
    {
        return PortableFastDot::Zero();
    }
    float FAST_DOT_ENTRY(FastDotZZO)
    {
        return PortableFastDot::Component<2>(Vec);
    }
    __declspec(naked) float FAST_DOT_ENTRY(FastDotZZG)
    {
        __asm
        {
            mov eax, dword ptr [esp + 4]
            fld dword ptr [eax + 8]
            fmul dword ptr [ecx + 8]
            ret 4
        }
    }
    float FAST_DOT_ENTRY(FastDotZOZ)
    {
        return PortableFastDot::Component<1>(Vec);
    }
    __declspec(naked) float FAST_DOT_ENTRY(FastDotZOO)
    {
        __asm
        {
            mov eax, dword ptr [esp + 4]
            fld dword ptr [eax + 8]
            fadd dword ptr [eax + 4]
            ret 4
        }
    }
    __declspec(naked) float FAST_DOT_ENTRY(FastDotZOG)
    {
        __asm
        {
            mov eax, dword ptr [esp + 4]
            fld dword ptr [eax + 8]
            fmul dword ptr [ecx + 8]
            fadd dword ptr [eax + 4]
            ret 4
        }
    }
    __declspec(naked) float FAST_DOT_ENTRY(FastDotZGZ)
    {
        __asm
        {
            mov eax, dword ptr [esp + 4]
            fld dword ptr [eax + 4]
            fmul dword ptr [ecx + 4]
            ret 4
        }
    }
    __declspec(naked) float FAST_DOT_ENTRY(FastDotZGO)
    {
        __asm
        {
            mov eax, dword ptr [esp + 4]
            fld dword ptr [eax + 4]
            fmul dword ptr [ecx + 4]
            fadd dword ptr [eax + 8]
            ret 4
        }
    }
    __declspec(naked) float FAST_DOT_ENTRY(FastDotZGG)
    {
        __asm
        {
            mov eax, dword ptr [esp + 4]
            fld dword ptr [eax + 8]
            fmul dword ptr [ecx + 8]
            fld dword ptr [eax + 4]
            fmul dword ptr [ecx + 4]
            faddp st(1), st
            ret 4
        }
    }
    float FAST_DOT_ENTRY(FastDotOZZ)
    {
        return PortableFastDot::Component<0>(Vec);
    }
    __declspec(naked) float FAST_DOT_ENTRY(FastDotOZO)
    {
        __asm
        {
            mov eax, dword ptr [esp + 4]
            fld dword ptr [eax + 8]
            fadd dword ptr [eax]
            ret 4
        }
    }
    __declspec(naked) float FAST_DOT_ENTRY(FastDotOZG)
    {
        __asm
        {
            mov eax, dword ptr [esp + 4]
            fld dword ptr [eax + 8]
            fmul dword ptr [ecx + 8]
            fadd dword ptr [eax]
            ret 4
        }
    }
    __declspec(naked) float FAST_DOT_ENTRY(FastDotOOZ)
    {
        __asm
        {
            mov eax, dword ptr [esp + 4]
            fld dword ptr [eax + 4]
            fadd dword ptr [eax]
            ret 4
        }
    }
    __declspec(naked) float FAST_DOT_ENTRY(FastDotOOO)
    {
        __asm
        {
            mov eax, dword ptr [esp + 4]
            fld dword ptr [eax + 8]
            fadd dword ptr [eax + 4]
            fadd dword ptr [eax]
            ret 4
        }
    }
    __declspec(naked) float FAST_DOT_ENTRY(FastDotOOG)
    {
        __asm
        {
            mov eax, dword ptr [esp + 4]
            fld dword ptr [eax + 8]
            fmul dword ptr [ecx + 8]
            fadd dword ptr [eax + 4]
            fadd dword ptr [eax]
            ret 4
        }
    }
    __declspec(naked) float FAST_DOT_ENTRY(FastDotOGZ)
    {
        __asm
        {
            mov eax, dword ptr [esp + 4]
            fld dword ptr [eax + 4]
            fmul dword ptr [ecx + 4]
            fadd dword ptr [eax]
            ret 4
        }
    }
    __declspec(naked) float FAST_DOT_ENTRY(FastDotOGO)
    {
        __asm
        {
            mov eax, dword ptr [esp + 4]
            fld dword ptr [eax + 4]
            fmul dword ptr [ecx + 4]
            fadd dword ptr [eax + 8]
            fadd dword ptr [eax]
            ret 4
        }
    }
    __declspec(naked) float FAST_DOT_ENTRY(FastDotOGG)
    {
        __asm
        {
            mov eax, dword ptr [esp + 4]
            fld dword ptr [eax + 8]
            fmul dword ptr [ecx + 8]
            fld dword ptr [eax + 4]
            fmul dword ptr [ecx + 4]
            faddp st(1), st
            fadd dword ptr [eax]
            ret 4
        }
    }
    __declspec(naked) float FAST_DOT_ENTRY(FastDotGZZ)
    {
        __asm
        {
            mov eax, dword ptr [esp + 4]
            fld dword ptr [eax]
            fmul dword ptr [ecx]
            ret 4
        }
    }
    __declspec(naked) float FAST_DOT_ENTRY(FastDotGZO)
    {
        __asm
        {
            mov eax, dword ptr [esp + 4]
            fld dword ptr [eax]
            fmul dword ptr [ecx]
            fadd dword ptr [eax + 8]
            ret 4
        }
    }
    __declspec(naked) float FAST_DOT_ENTRY(FastDotGZG)
    {
        __asm
        {
            mov eax, dword ptr [esp + 4]
            fld dword ptr [eax + 8]
            fmul dword ptr [ecx + 8]
            fld dword ptr [eax]
            fmul dword ptr [ecx]
            faddp st(1), st
            ret 4
        }
    }
    __declspec(naked) float FAST_DOT_ENTRY(FastDotGOZ)
    {
        __asm
        {
            mov eax, dword ptr [esp + 4]
            fld dword ptr [eax]
            fmul dword ptr [ecx]
            fadd dword ptr [eax + 4]
            ret 4
        }
    }
    __declspec(naked) float FAST_DOT_ENTRY(FastDotGOO)
    {
        __asm
        {
            mov eax, dword ptr [esp + 4]
            fld dword ptr [eax]
            fmul dword ptr [ecx]
            fadd dword ptr [eax + 8]
            fadd dword ptr [eax + 4]
            ret 4
        }
    }
    __declspec(naked) float FAST_DOT_ENTRY(FastDotGOG)
    {
        __asm
        {
            mov eax, dword ptr [esp + 4]
            fld dword ptr [eax + 8]
            fmul dword ptr [ecx + 8]
            fld dword ptr [eax]
            fmul dword ptr [ecx]
            faddp st(1), st
            fadd dword ptr [eax + 4]
            ret 4
        }
    }
    __declspec(naked) float FAST_DOT_ENTRY(FastDotGGZ)
    {
        __asm
        {
            mov eax, dword ptr [esp + 4]
            fld dword ptr [eax + 4]
            fmul dword ptr [ecx + 4]
            fld dword ptr [eax]
            fmul dword ptr [ecx]
            faddp st(1), st
            ret 4
        }
    }
    __declspec(naked) float FAST_DOT_ENTRY(FastDotGGO)
    {
        __asm
        {
            mov eax, dword ptr [esp + 4]
            fld dword ptr [eax + 4]
            fmul dword ptr [ecx + 4]
            fld dword ptr [eax]
            fmul dword ptr [ecx]
            faddp st(1), st
            fadd dword ptr [eax + 8]
            ret 4
        }
    }
    __declspec(naked) float FAST_DOT_ENTRY(FastDotGGG)
    {
        __asm
        {
            mov eax, dword ptr [esp + 4]
            fld dword ptr [eax + 8]
            fmul dword ptr [ecx + 8]
            fld dword ptr [eax + 4]
            fmul dword ptr [ecx + 4]
            faddp st(1), st
            fld dword ptr [eax]
            fmul dword ptr [ecx]
            faddp st(1), st
            ret 4
        }
    }
#else
    float FFastDot::FastDotZZZ(const FVector*) { return 0.0f; }
    float FFastDot::FastDotZZO(const FVector* Vec) { return Vec->Z; }
    float FFastDot::FastDotZZG(const FVector* Vec) { return Vec->Z * C; }
    float FFastDot::FastDotZOZ(const FVector* Vec) { return Vec->Y; }
    float FFastDot::FastDotZOO(const FVector* Vec) { return Vec->Z + Vec->Y; }
    float FFastDot::FastDotZOG(const FVector* Vec) { return Vec->Z * C + Vec->Y; }
    float FFastDot::FastDotZGZ(const FVector* Vec) { return Vec->Y * B; }
    float FFastDot::FastDotZGO(const FVector* Vec) { return Vec->Y * B + Vec->Z; }
    float FFastDot::FastDotZGG(const FVector* Vec) { return Vec->Z * C + Vec->Y * B; }
    float FFastDot::FastDotOZZ(const FVector* Vec) { return Vec->X; }
    float FFastDot::FastDotOZO(const FVector* Vec) { return Vec->Z + Vec->X; }
    float FFastDot::FastDotOZG(const FVector* Vec) { return Vec->Z * C + Vec->X; }
    float FFastDot::FastDotOOZ(const FVector* Vec) { return Vec->Y + Vec->X; }
    float FFastDot::FastDotOOO(const FVector* Vec) { return Vec->Z + Vec->Y + Vec->X; }
    float FFastDot::FastDotOOG(const FVector* Vec) { return Vec->Z * C + Vec->Y + Vec->X; }
    float FFastDot::FastDotOGZ(const FVector* Vec) { return Vec->Y * B + Vec->X; }
    float FFastDot::FastDotOGO(const FVector* Vec) { return Vec->Y * B + Vec->Z + Vec->X; }
    float FFastDot::FastDotOGG(const FVector* Vec) { return Vec->Z * C + Vec->Y * B + Vec->X; }
    float FFastDot::FastDotGZZ(const FVector* Vec) { return Vec->X * A; }
    float FFastDot::FastDotGZO(const FVector* Vec) { return Vec->X * A + Vec->Z; }
    float FFastDot::FastDotGZG(const FVector* Vec) { return Vec->Z * C + Vec->X * A; }
    float FFastDot::FastDotGOZ(const FVector* Vec) { return Vec->X * A + Vec->Y; }
    float FFastDot::FastDotGOO(const FVector* Vec) { return Vec->X * A + Vec->Z + Vec->Y; }
    float FFastDot::FastDotGOG(const FVector* Vec) { return Vec->Z * C + Vec->X * A + Vec->Y; }
    float FFastDot::FastDotGGZ(const FVector* Vec) { return Vec->Y * B + Vec->X * A; }
    float FFastDot::FastDotGGO(const FVector* Vec) { return Vec->Y * B + Vec->X * A + Vec->Z; }
    float FFastDot::FastDotGGG(const FVector* Vec) { return Vec->Z * C + Vec->Y * B + Vec->X * A; }
#endif

    const FFastDotKernel G_FastDotKernels[27] = {
        FAST_DOT_ADDRESS(FastDotZZZ), FAST_DOT_ADDRESS(FastDotZZO), FAST_DOT_ADDRESS(FastDotZZG), FAST_DOT_ADDRESS(FastDotZOZ),
        FAST_DOT_ADDRESS(FastDotZOO), FAST_DOT_ADDRESS(FastDotZOG), FAST_DOT_ADDRESS(FastDotZGZ), FAST_DOT_ADDRESS(FastDotZGO),
        FAST_DOT_ADDRESS(FastDotZGG), FAST_DOT_ADDRESS(FastDotOZZ), FAST_DOT_ADDRESS(FastDotOZO), FAST_DOT_ADDRESS(FastDotOZG),
        FAST_DOT_ADDRESS(FastDotOOZ), FAST_DOT_ADDRESS(FastDotOOO), FAST_DOT_ADDRESS(FastDotOOG), FAST_DOT_ADDRESS(FastDotOGZ),
        FAST_DOT_ADDRESS(FastDotOGO), FAST_DOT_ADDRESS(FastDotOGG), FAST_DOT_ADDRESS(FastDotGZZ), FAST_DOT_ADDRESS(FastDotGZO),
        FAST_DOT_ADDRESS(FastDotGZG), FAST_DOT_ADDRESS(FastDotGOZ), FAST_DOT_ADDRESS(FastDotGOO), FAST_DOT_ADDRESS(FastDotGOG),
        FAST_DOT_ADDRESS(FastDotGGZ), FAST_DOT_ADDRESS(FastDotGGO), FAST_DOT_ADDRESS(FastDotGGG)};

#undef FAST_DOT_ENTRY
#undef FAST_DOT_ADDRESS

    const int G_FastDotSavedOps[27] = {5, 5, 4, 5, 4, 3, 4, 3, 2, 5, 4, 3, 4, 3, 2, 3, 2, 1, 4, 3, 2, 3, 2, 1, 2, 1, 0};

    static_assert(offsetof(FFastDot, A) == 0, "FastDot A");
    static_assert(offsetof(FFastDot, B) == 4, "FastDot B");
    static_assert(offsetof(FFastDot, C) == 8, "FastDot C");
    static_assert(offsetof(FFastDot, Kernel) == 0xC, "retail kernel @ +0xC");
    static_assert(offsetof(FFastDot, SavedOps) == 0x10, "retail saved-ops @ +0x10");
    static_assert(sizeof(FFastDotKernel) == 4, "MSVC simple-class member pointer is a code address");

    // Ordinary C++ member dispatch; the native result/store boundary is
    // checked against 71ab4..71ad2 and 72110..7213c.
    void StoreKernelPlain(FFastDot& Dot, const FVector* Src, float* Dst)
    {
#if defined(__clang__) && defined(_M_IX86)
        *Dst = Dot.Kernel(&Dot, nullptr, Src);
#else
        *Dst = (Dot.*Dot.Kernel)(Src);
#endif
    }

    void StoreKernelAdd(FFastDot& Dot, const FVector* Src, float* Dst, const float* Addend)
    {
#if defined(__clang__) && defined(_M_IX86)
        *Dst = Dot.Kernel(&Dot, nullptr, Src) + *Addend;
#else
        *Dst = (Dot.*Dot.Kernel)(Src) + *Addend;
#endif
    }

    void BindFastDot(FFastDot& Dot, float A, float B, float C)
    {
        Dot.A = A;
        Dot.B = B;
        Dot.C = C;
        const int KernelId = ClassifyLane(A) * 9 + ClassifyLane(B) * 3 + ClassifyLane(C);
        Dot.SavedOps = G_FastDotSavedOps[KernelId];
        Dot.Kernel = G_FastDotKernels[KernelId];
    }

    void TransformListFast(FVector* Dst, const FMatrix& Matrix, const FVector* Source, int Count,
                           const FVector* Translation)
    {
        FFastDot Row0;
        FFastDot Row1;
        FFastDot Row2;
        BindFastDot(Row0, Matrix.D[0][0], Matrix.D[0][1], Matrix.D[0][2]);
        BindFastDot(Row1, Matrix.D[1][0], Matrix.D[1][1], Matrix.D[1][2]);
        BindFastDot(Row2, Matrix.D[2][0], Matrix.D[2][1], Matrix.D[2][2]);

        const FVector* End = Source + Count;
        const FVector* Src = Source;
        FVector* Out = Dst;

        if (Row0.SavedOps + Row1.SavedOps + Row2.SavedOps < 4)
        {
            while (Src < End)
            {
                const float X = Src->X;
                const float Y = Src->Y;
                const float Z = Src->Z;
                if (Translation != nullptr)
                {
                    // 7209b..720e8: the FIRST row adds X/Z/Y; rows 1/2 add Z/Y/X.
                    // Each destination is written before reading the next row.
                    Out->X = ((X * Matrix.D[0][0] + Z * Matrix.D[0][2]) +
                              Y * Matrix.D[0][1]) + Translation->X;
                    Out->Y = ((Z * Matrix.D[1][2] + Y * Matrix.D[1][1]) +
                              X * Matrix.D[1][0]) + Translation->Y;
                    Out->Z = ((Z * Matrix.D[2][2] + Y * Matrix.D[2][1]) +
                              X * Matrix.D[2][0]) + Translation->Z;
                }
                else
                {
                    // 71a49..71a8d: Z/Y/X addition order, one final float store.
                    Out->X = (Z * Matrix.D[0][2] + Y * Matrix.D[0][1]) + X * Matrix.D[0][0];
                    Out->Y = (Z * Matrix.D[1][2] + Y * Matrix.D[1][1]) + X * Matrix.D[1][0];
                    Out->Z = (Z * Matrix.D[2][2] + Y * Matrix.D[2][1]) + X * Matrix.D[2][0];
                }
                ++Src;
                ++Out;
            }
            return;
        }

        if (Translation != nullptr)
        {
            while (Src < End)
            {
                StoreKernelAdd(Row0, Src, &Out->X, &Translation->X);
                StoreKernelAdd(Row1, Src, &Out->Y, &Translation->Y);
                StoreKernelAdd(Row2, Src, &Out->Z, &Translation->Z);
                ++Src;
                ++Out;
            }
            return;
        }

        while (Src < End)
        {
            StoreKernelPlain(Row0, Src, &Out->X);
            StoreKernelPlain(Row1, Src, &Out->Y);
            StoreKernelPlain(Row2, Src, &Out->Z);
            ++Src;
            ++Out;
        }
    }
}
