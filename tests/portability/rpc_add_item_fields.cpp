#include "../../src/include/portable_rpc_item_fields.h"
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <xmmintrin.h>

struct Fields
{
    unsigned int id;
    int count;
    float health;
    unsigned char mounted;
    unsigned char guard[3];
};
static_assert(sizeof(Fields) == 16, "Three dwords, mounted byte and canaries");

__declspec(noinline) void Reference(Fields* output, unsigned int id, int count,
                                   float Health, unsigned char mounted)
{
    __asm {
        mov ecx, output
        fld Health
#if defined(TEST_MODIFY_ITEM)
        movzx eax, word ptr id
#else
        mov eax, id
#endif
        mov dword ptr [ecx], eax
        mov eax, count
        mov dword ptr [ecx+4], eax
        mov al, mounted
        mov byte ptr [ecx+12], al
        fstp dword ptr [ecx+8]
    }
}

__declspec(noinline) void Candidate(Fields* output, unsigned int id, int count,
                                   float health, unsigned char mounted)
{
#if defined(TEST_MODIFY_ITEM)
    id = static_cast<unsigned short>(id);
#endif
    PortableRpcItemFields::Store(&output->id, &output->count, &output->health,
                                &output->mounted, id, count, &health, mounted);
}

int main()
{
    const std::uint32_t patterns[] = {0, 0x80000000u, 0x3f800000u, 0xbf800000u,
        0x7f800000u, 0xff800000u, 0x7fc12345u, 0xffc54321u, 0x7f812345u,
        0xff854321u, 1, 0x80000001u, 0x007fffffu, 0x00800000u, 0x7f7fffffu};
    unsigned short original;
    __asm { fnstcw original }
    const unsigned short precisions[] = {0, 0x200, 0x300};
    std::uint32_t random = 0x715237u;
    unsigned cases = 0;
    for (unsigned short pc : precisions)
        for (unsigned short rc = 0; rc < 0x1000; rc += 0x400)
    {
        const unsigned short control = static_cast<unsigned short>((original & ~0xf00) | pc | rc | 0x3f);
        __asm { fldcw control }
        for (unsigned sample = 0; sample < 1024; ++sample)
        {
            random ^= random << 13; random ^= random >> 17; random ^= random << 5;
            const unsigned int id = random;
            int count;
            std::memcpy(&count, &random, sizeof(count));
            const auto bits = sample < 15 ? patterns[sample] : random;
            float health;
            std::memcpy(&health, &bits, sizeof(health));
            const auto mounted = static_cast<unsigned char>(sample);
            Fields a, b;
            std::memset(&a, 0xa5, sizeof(a));
            std::memcpy(&b, &a, sizeof(a));
            unsigned short expected, actual, after;
            const unsigned mxcsr = _mm_getcsr();
            __asm { fnclex }
            Reference(&a, id, count, health, mounted);
            __asm { fnstsw expected }
            __asm { fnclex }
            Candidate(&b, id, count, health, mounted);
            __asm { fnstsw actual }
            __asm { fnstcw after }
            if (std::memcmp(&a, &b, sizeof(a)) || ((expected ^ actual) & 0x383f) != 0 ||
                after != control || mxcsr != _mm_getcsr())
            {
                std::fprintf(stderr, "Mismatch sample=%u pc=%x rc=%x status=%x/%x\n", sample, pc, rc, expected, actual);
                return 1;
            }
            ++cases;
        }
    }
    __asm { fnclex }
    __asm { fldcw original }
    std::printf("PASS: %u exact field-buffer/exception/stack comparisons; control and MXCSR unchanged\n", cases);
    return 0;
}
