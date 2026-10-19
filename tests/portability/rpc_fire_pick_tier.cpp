#include "../../src/include/portable_rpc_quantization.h"
#include <cstdio>
#include <cstring>
#include <xmmintrin.h>
static const double GFireSmallLimit = 64.0;
static const double GFireMediumLimit = 8192.0;

__declspec(noinline) unsigned char Reference(const float& Value, bool bHasMediumTier) {
    const float* Source = &Value;
    unsigned char Tier;
    if (bHasMediumTier) {
        __asm {
            mov ecx, Source
            xor edx, edx
            fld dword ptr [ecx]
            fabs
            fcom qword ptr GFireSmallLimit
            fnstsw ax
            test ah, 5
            jp FireTryMedium
            fstp st(0)
            mov dl, 1
            jmp FireTierDone
        FireTryMedium:
            fcomp qword ptr GFireMediumLimit
            fnstsw ax
            test ah, 5
            jp FireTierDone
            mov dl, 2
        FireTierDone:
            mov Tier, dl
        }
    } else {
        __asm {
            mov ecx, Source
            xor edx, edx
            fld dword ptr [ecx]
            fabs
            fcomp qword ptr GFireSmallLimit
            fnstsw ax
            test ah, 5
            jp FireSmallDone
            mov dl, 1
        FireSmallDone:
            mov Tier, dl
        }
    }
    return Tier;
}

__declspec(noinline) unsigned char Candidate(const float& value, bool hasMediumTier)
{
    return PortableRpcQuantization::PickFireTier(&value, hasMediumTier);
}

int main()
{
    const std::uint32_t patterns[] = {0, 0x80000000u, 1, 0x80000001u,
        0x007fffffu, 0x00800000u, 0x3f7fffffu, 0x3f800000u, 0x3f800001u,
        0x3f000000u, 0xbf000000u, 0x5b800000u, 0xdb800000u, 0x7f7fffffu,
        0xff7fffffu, 0x7f800000u, 0xff800000u, 0x7fc12345u, 0x7f812345u,
        0x3c010203u, 0xbc010203u, 0x3c010204u, 0xbc010204u, 0x3c010205u, 0xbc010205u, 0x5b810203u, 0xdb810203u, 0x5b810204u, 0xdb810204u, 0x5b810205u, 0xdb810205u, 0x3b808080u, 0xbb808080u, 0x3b808081u, 0xbb808081u, 0x3b808082u, 0xbb808082u, 0x5b008080u, 0xdb008080u, 0x5b008081u, 0xdb008081u, 0x5b008082u, 0xdb008082u,
        0x427fffffu, 0xc27fffffu, 0x42800000u, 0xc2800000u, 0x42800001u, 0xc2800001u,
        0x45ffffffu, 0xc5ffffffu, 0x46000000u, 0xc6000000u, 0x46000001u, 0xc6000001u,
        0x3f010203u, 0xbf010203u, 0x3f010204u, 0xbf010204u, 0x3f010205u, 0xbf010205u,
        0x3e8000ffu, 0xbe8000ffu, 0x3e800100u, 0xbe800100u, 0x3e800101u, 0xbe800101u,
        0x5e810203u, 0xde810203u, 0x5e810204u, 0xde810204u, 0x5e810205u, 0xde810205u,
        0x5e0000ffu, 0xde0000ffu, 0x5e000100u, 0xde000100u, 0x5e000101u, 0xde000101u};
    unsigned short original;
    __asm { fnstcw original }
    const unsigned short precisions[] = {0, 0x200, 0x300};
    std::uint32_t random = 0x48127acu;
    unsigned cases = 0, outputs = 0, flags = 0, controls = 0, mxcsrChanges = 0;
    for (unsigned short pc : precisions)
        for (unsigned short rc = 0; rc < 0x1000; rc += 0x400)
    {
        const unsigned short control = static_cast<unsigned short>((original & ~0xf00) | pc | rc | 0x3f);
        __asm { fldcw control }
        for (unsigned sample = 0; sample < 4096; ++sample)
            for (unsigned medium = 0; medium != 2; ++medium)
        {
            random ^= random << 13; random ^= random >> 7; random ^= random << 17;
            const std::uint32_t bits = sample < sizeof(patterns) / sizeof(patterns[0]) ? patterns[sample] : random;
            float input;
            std::memcpy(&input, &bits, sizeof(input));
            unsigned short expectedStatus, actualStatus, after;
            const unsigned mxcsr = _mm_getcsr();
            __asm { fnclex }
            const auto expected = Reference(input, medium != 0);
            __asm { fnstsw expectedStatus }
            __asm { fnclex }
            const auto actual = Candidate(input, medium != 0);
            __asm { fnstsw actualStatus }
            __asm { fnstcw after }
            outputs += expected != actual;
            flags += ((expectedStatus ^ actualStatus) & 0x383f) != 0;
            controls += after != control;
            mxcsrChanges += mxcsr != _mm_getcsr();
            ++cases;
        }
    }
    __asm { fnclex }
    __asm { fldcw original }
    std::printf("%u comparisons: %u result differences, %u exception differences, %u control changes, %u MXCSR changes\n", cases, outputs, flags, controls, mxcsrChanges);
    return outputs || flags || controls || mxcsrChanges ? 1 : 0;
}
