#include "softfloat-sources.h"
#include <cstdio>
#include <cstdint>

static extFloat80_t RawMemoryOperand(std::uint32_t bits)
{
    int exponent = (bits >> 23) & 0xff;
    std::uint32_t fraction = bits & 0x7fffff;
    extFloat80_t result{};
    result.signExp = static_cast<std::uint16_t>((bits >> 16) & 0x8000);
    if (exponent == 255) {
        result.signExp |= 0x7fff;
        result.signif = 0x8000000000000000ull | (std::uint64_t(fraction) << 40);
    } else if (exponent || fraction) {
        if (!exponent) {
            exponent=1;
            while (!(fraction & 0x800000)) { fraction <<= 1; --exponent; }
        }
        result.signExp |= static_cast<std::uint16_t>(exponent + 0x3f80);
        result.signif = std::uint64_t(fraction | 0x800000) << 40;
    }
    return result;
}

enum Operation { Add, Subtract, Multiply, Divide, SquareRoot };
struct Encoded80 { unsigned char bytes[10]; };

__declspec(noinline) static void ReferenceWide(Operation operation, std::uint32_t a, std::uint32_t b, Encoded80* output)
{
    if (operation == Add) { __asm {
        fld dword ptr a
        fadd dword ptr b
    }} else if (operation == Subtract) { __asm {
        fld dword ptr a
        fsub dword ptr b
    }} else if (operation == Multiply) { __asm {
        fld dword ptr a
        fmul dword ptr b
    }} else if (operation == Divide) { __asm {
        fld dword ptr a
        fdiv dword ptr b
    }} else { __asm {
        fld dword ptr a
        fsqrt
    }}
    __asm {
        mov eax, output
        fstp tbyte ptr [eax]
    }
}

__declspec(noinline) static std::uint32_t Reference(Operation operation, std::uint32_t a, std::uint32_t b)
{
    std::uint32_t result;
    if (operation == Add) { __asm {
        fld dword ptr a
        fadd dword ptr b
        fstp dword ptr result
    }} else if (operation == Subtract) { __asm {
        fld dword ptr a
        fsub dword ptr b
        fstp dword ptr result
    }} else if (operation == Multiply) { __asm {
        fld dword ptr a
        fmul dword ptr b
        fstp dword ptr result
    }} else if (operation == Divide) { __asm {
        fld dword ptr a
        fdiv dword ptr b
        fstp dword ptr result
    }} else { __asm {
        fld dword ptr a
        fsqrt
        fstp dword ptr result
    }}
    return result;
}

static std::uint32_t Software(Operation operation, std::uint32_t a, std::uint32_t b, Encoded80* wide)
{
    softfloat_exceptionFlags=0;
    auto left=f32_to_extF80(float32_t{a}); // FLD quiets the register operand.
    auto right=RawMemoryOperand(b);      // FADD's memory operand is not pre-quieted.
    extFloat80_t result;
    switch (operation) {
    case Add: result=extF80_add(left,right); break;
    case Subtract: result=extF80_sub(left,right); break;
    case Multiply: result=extF80_mul(left,right); break;
    case Divide: result=extF80_div(left,right); break;
    default: result=extF80_sqrt(left); break;
    }
    for (unsigned i=0;i<8;++i) wide->bytes[i]=static_cast<unsigned char>(result.signif>>(8*i));
    wide->bytes[8]=static_cast<unsigned char>(result.signExp);
    wide->bytes[9]=static_cast<unsigned char>(result.signExp>>8);
    return extF80_to_f32(result).v;
}

int main()
{
    unsigned short old;
    __asm { fnstcw old }
    unsigned failures=0;
    std::uint32_t random=0xb193425;
    const std::uint32_t patterns[]={0,0x80000000u,0x3f800000u,0xbf800000u,0x7f800000u,0xff800000u,
        0x7fc12345u,0xffc54321u,0x7f812345u,0xff854321u,1,0x80000001u,0x007fffffu,0x00800000u,0x7f7fffffu};
    const unsigned char rounds[]={softfloat_round_near_even,softfloat_round_min,softfloat_round_max,softfloat_round_minMag};
    const unsigned char precisions[]={32,64,80};
    const char* names[]={"add","subtract","multiply","divide","square-root"};
    for (unsigned operationIndex=0;operationIndex<5;++operationIndex) {
    const auto operation=static_cast<Operation>(operationIndex);
    unsigned mismatches=0, wideMismatches=0, flagMismatches=0, cases=0;
    for (unsigned pc=0;pc<3;++pc) for(unsigned rc=0;rc<4;++rc) {
        extF80_roundingPrecision=precisions[pc]; softfloat_roundingMode=rounds[rc];
        const unsigned short control=static_cast<unsigned short>((old&~0xf00)|(pc==0?0:pc==1?0x200:0x300)|(rc<<10)|0x3f);
        __asm { fldcw control }
        for(unsigned i=0;i<100000;++i) {
            random^=random<<13;random^=random>>17;random^=random<<5;
            auto a=i<225?patterns[i/15]:random;
            random^=random<<13;random^=random>>17;random^=random<<5;
            auto b=i<225?patterns[i%15]:random;
            __asm { fnclex }
            const auto expected=Reference(operation,a,b);
            unsigned short status;
            __asm { fnstsw status }
            Encoded80 wideA, wideB;
            ReferenceWide(operation,a,b,&wideA);
            const auto actual=Software(operation,a,b,&wideB);
            bool wideEqual=true;
            for(unsigned byte=0;byte<10;++byte) if(wideA.bytes[byte]!=wideB.bytes[byte]) wideEqual=false;
            if(!wideEqual) ++wideMismatches;
            if(expected!=actual) {
                if(mismatches<3) std::printf("Mismatch a=%08x b=%08x pc=%u rc=%u %08x/%08x\n",a,b,pc,rc,expected,actual);
                ++mismatches;
            }
            const unsigned flags=softfloat_exceptionFlags;
            const unsigned mapped=(flags&softfloat_flag_invalid?1:0)|(flags&softfloat_flag_infinite?4:0)|
                (flags&softfloat_flag_overflow?8:0)|(flags&softfloat_flag_underflow?16:0)|(flags&softfloat_flag_inexact?32:0);
            if((status&0x3d)!=mapped) ++flagMismatches;
            ++cases;
        }
    }
    std::printf("%s: %u cases: %u float32 bit mismatches, %u binary80 bit mismatches, %u standard exception-flag mismatches (excludes x87 DE/C1/trap delivery)\n",names[operationIndex],cases,mismatches,wideMismatches,flagMismatches);
    failures += mismatches + wideMismatches + flagMismatches;
    }
    __asm { fnclex }
    __asm { fldcw old }
    return failures?1:0;
}
