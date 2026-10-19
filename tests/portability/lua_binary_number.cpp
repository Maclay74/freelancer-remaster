#include "../../src/Thorn/lua32/src/PortableNumberMath.h"
#include "candidates/legacy_lua_binary.h"
#include <cstdio>
#include <cstdint>
#include <cstring>

__declspec(noinline) static void Candidate(double* left,const double* right,int divide)
{
    ThornLuaBinaryNumber(left,right,divide);
}
int main()
{
    unsigned short original;
    __asm { fnstcw original }
    const unsigned short measurement=static_cast<unsigned short>((original&~0xf00)|0x200|0x3f);
    const unsigned short precisions[]={0,0x200,0x300};
    const std::uint64_t special[]={0,0x8000000000000000ull,1,0x000fffffffffffffull,
        0x0010000000000000ull,0x3ff0000000000000ull,0xbff0000000000000ull,
        0x7fefffffffffffffull,0x7ff0000000000000ull,0xfff0000000000000ull,
        0x7ff8000000000001ull,0xfff8000000000001ull,0x7ff0000000000001ull,
        0xfff0000000000001ull,0x7ff8abcdef123456ull,0x7ff0abcdef123456ull};
    std::uint32_t random=0x8f312bu;
    unsigned failures=0,cases=0;
    for (unsigned short pc:precisions) for (unsigned short rc=0;rc<0x1000;rc+=0x400)
        for (unsigned sample=0;sample<4096;++sample) {
        __asm { fldcw measurement }
        random^=random<<13;random^=random>>17;random^=random<<5;
        const std::uint64_t a=sample<256 ? special[sample/16] :
            (static_cast<std::uint64_t>(random)<<32)|(random^0x2ab384u);
        random^=random<<13;random^=random>>17;random^=random<<5;
        const std::uint64_t b=sample<256 ? special[sample%16] :
            (static_cast<std::uint64_t>(random)<<32)|(random^0xe34c12u);
        const unsigned short control=static_cast<unsigned short>((original&~0xf00)|pc|rc|0x3f);
        for (int divide=0;divide<2;++divide) for (int alias=0;alias<2;++alias) {
            double old[2],now[2];
            std::memcpy(old,&a,8);std::memcpy(old+1,&b,8);std::memcpy(now,old,16);
            __asm { fldcw control }
            if (divide) LegacyLuaBinary::Divide(old,old+(alias?0:1));
            else LegacyLuaBinary::Subtract(old,old+(alias?0:1));
            Candidate(now,now+(alias?0:1),divide);
            unsigned short after;
            __asm { fnstcw after }
            __asm { fldcw measurement }
            if ((std::memcmp(old,now,16)!=0 || after!=control) && failures++<8) {
                std::uint64_t x,y;std::memcpy(&x,old,8);std::memcpy(&y,now,8);
                std::printf("binary mismatch pc=%x rc=%x sample=%u divide=%d alias=%d a=%llx b=%llx old=%llx new=%llx\n",
                    pc,rc,sample,divide,alias,a,b,x,y);
            }
            ++cases;
        }
    }
    __asm { fnclex }
    __asm { fldcw original }
    std::printf("%u exact binary-operation cases; %u failures\n",cases,failures);
    return failures?1:0;
}
