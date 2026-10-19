// Retained pre-refactor table implementation, used only as a comparison oracle.
#pragma once
#include <cmath>
namespace LegacyScalarMath
{
    // DAT_06f74030..06f7442f: 0x400 bytes (06f74430 is a separate guard byte). Readers index
    // with (bits >> 14) & 0x3FF (06f72851/06f728d1).
    unsigned char G_InvSqrtTable[0x400];

    // Retail PE .data at DAT_06f74030 is BSS (on-disk zeros). FUN_06f71010 / DllMain fill all
    // 0x400 entries via FSQRT + FDIVR qword 1.0, then (bits + 0x2000) >> 15. Match that x87 path.
    void BuildInvSqrtTable()
    {
        const double One = 1.0;
        for (unsigned int Index = 0; Index < 0x400; ++Index)
        {
            unsigned int Bits = (Index | 0xFC00u) << 14;
            unsigned int RecipBits = 0;
#if defined(_M_IX86)
            __asm
            {
                fld dword ptr [Bits]
                fsqrt
                fdivr qword ptr [One]
                fstp dword ptr [RecipBits]
            }
#else
            union
            {
                unsigned int AsBits;
                float Value;
            } Pun;
            Pun.AsBits = Bits;
            Pun.Value = static_cast<float>(1.0 / std::sqrt(static_cast<double>(Pun.Value)));
            RecipBits = Pun.AsBits;
#endif
            G_InvSqrtTable[Index] = static_cast<unsigned char>((RecipBits + 0x2000u) >> 15);
        }
        // 06f7104c / 06f710fb: MOV byte [06f74230],0xFF = base + 0x200. Index 0x200 is the
        // mantissa-1.0 slot (X = 1.0, 4.0, ...): its computed byte wraps to 0x00, retail patches it.
        G_InvSqrtTable[0x200] = 0xFF;
    }

    float TableInvSqrt(float X)
    {
        union
        {
            unsigned int Bits;
            float Value;
        } Pun;
        Pun.Value = X;
        unsigned int Exponent = (Pun.Bits >> 23) & 0xFFu;
        const unsigned int TableIndex = (Pun.Bits >> 14) & 0x3FFu;
        unsigned int Guess = (0x5F000000u - (Exponent << 22)) & 0xFF800000u;
        Guess |= static_cast<unsigned int>(G_InvSqrtTable[TableIndex]) << 15;
        Pun.Bits = Guess;
        float Y = Pun.Value;
        Y = (3.0f - Y * Y * X) * Y * 0.5f;
        Y = (3.0f - Y * Y * X) * Y * 0.5f;
        return Y;
    }

}
