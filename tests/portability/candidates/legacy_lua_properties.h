#pragma once

namespace LegacyLuaProperties
{
    __declspec(noinline) int LuaMillisecondsToRetailInt(lua_Object Object)
    {
        const double Scale = 1000.0;
        unsigned short OldControl;
        unsigned short ChopControl;
        long long Result;
        const double Number = lua_getnumber(Object);
        __asm {
            fld qword ptr Number
            fmul qword ptr Scale
            fnstcw OldControl
            mov ax, OldControl
            or ax, 0c00h
            mov ChopControl, ax
            fldcw ChopControl
            fistp qword ptr Result
            fldcw OldControl
        }
        return static_cast<int>(static_cast<unsigned int>(Result));
    }

    __declspec(noinline) int ReadSceneStartMilliseconds(lua_Object Object)
    {
        const int bNil = lua_isnil(Object);
        const unsigned int ScaleBits = 0x447a0000u;
        unsigned short OldControl;
        unsigned short ChopControl;
        long long Result;
        const double Number = bNil != 0 ? 0.0 : lua_getnumber(Object);
        __asm {
            fld qword ptr Number
            fmul dword ptr ScaleBits
            fwait
            fnstcw OldControl
            fwait
            mov ax, OldControl
            or ax, 0c00h
            mov ChopControl, ax
            fldcw ChopControl
            fistp qword ptr Result
            fldcw OldControl
        }
        return static_cast<int>(static_cast<unsigned int>(Result));
    }

    __declspec(noinline) void ApplyLightEventPost(FThornLightProps* Props, int* Mask)
    {
        const int EntryMask = *Mask;
        if ((EntryMask & kThornLightBitColor) != 0)
        {
            const unsigned int ColorScaleBits = 0x3b808081u;
            const int* Colors = Props->Color;
            float* Diffuse = Props->Diffuse;
            unsigned int ColorBits[4];
            // Actual C++ Debug/Release publishes R/G before FILD blue and
            // B/alpha before mask800. Retail24b80..95 instead publishes R/G
            // after FILD blue, then mask800 before its waiting FSTP32.
            __asm
            {
                mov ecx, Colors
                mov edx, Diffuse
                lea ebx, ColorBits
                mov eax, EntryMask
                fild dword ptr [ecx]
                mov dword ptr [ebx + 0ch], 03f800000h
                or eax, 0800h
                fmul dword ptr ColorScaleBits
                fstp dword ptr [ebx]
                mov esi, dword ptr [ebx]
                fild dword ptr [ecx + 4]
                fmul dword ptr ColorScaleBits
                fstp dword ptr [ebx + 4]
                fild dword ptr [ecx + 8]
                mov dword ptr [edx], esi
                mov esi, dword ptr [ebx + 4]
                mov dword ptr [edx + 4], esi
                fmul dword ptr ColorScaleBits
                mov ecx, Mask
                mov dword ptr [ecx], eax
                fstp dword ptr [ebx + 8]
                mov esi, dword ptr [ebx + 8]
                mov dword ptr [edx + 8], esi
                mov esi, dword ptr [ebx + 0ch]
                mov dword ptr [edx + 0ch], esi
            }
        }
        const int InfiniteMask = *Mask;
        if ((InfiniteMask & kThornLightBitInfinite) != 0)
        {
            Props->Type = (Props->Infinite == 0) ? 2 : 3;
            *Mask = (InfiniteMask & ~kThornLightBitInfinite) | kThornLightBitType;
        }
        // FCOMP +0.0, TEST AH,05, JP skips greater/equal/unordered.
        // Only ordered less (C0=1,C2=0, odd parity) falls through here.
        const int CutoffMask = *Mask;
        if ((CutoffMask & kThornLightBitCutoff) != 0)
        {
            const unsigned int ZeroBits = 0;
            const float* Cutoff = &Props->Cutoff;
            unsigned char bOrderedNegative;
            // Actual optimized C++ compares zero to cutoff and executes a
            // waiting FSTP on the unordered return path. Retail24be4..c10
            // has no waiting instruction after FNSTSW/TEST AH,05/JP.
            __asm
            {
                mov ecx, Cutoff
                fld dword ptr [ecx]
                fcomp dword ptr ZeroBits
                fnstsw ax
                test ah, 05h
                setnp bOrderedNegative
            }
            if (bOrderedNegative != 0)
            {
                Props->Type = 1;
                std::memcpy(&Props->Cutoff, &ZeroBits, sizeof(ZeroBits));
                std::memcpy(&Props->Theta, &ZeroBits, sizeof(ZeroBits));
                *Mask = CutoffMask | kThornLightBitType;
            }
        }
    }
}
