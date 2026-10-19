#include "../../src/Thorn/ExtendedProgressCodec.h"
#include <cstdio>

__declspec(noinline) static void LegacyStore(const double* input,unsigned char* output)
{
    __asm {
        mov eax,input
        fld qword ptr [eax]
        mov eax,output
        fstp tbyte ptr [eax]
    }
}
__declspec(noinline) static void LegacyLoad(const unsigned char* input,void* output,bool single)
{
    __asm {
        mov eax,input
        fld tbyte ptr [eax]
        mov eax,output
        cmp single,0
        je CodecDouble
        fstp dword ptr [eax]
        jmp CodecDone
    CodecDouble:
        fstp qword ptr [eax]
    CodecDone:
    }
}
__declspec(noinline) static void CandidateStore(const double* input,unsigned char* output)
{
    ThornExtendedProgress::StoreDouble(output,*input);
}
__declspec(noinline) static void CandidateLoad(const unsigned char* input,void* output,bool single)
{
    if (single) {
        const float value=ThornExtendedProgress::LoadFloat(input);
        std::memcpy(output,&value,sizeof(value));
    } else {
        const double value=ThornExtendedProgress::LoadDouble(input);
        std::memcpy(output,&value,sizeof(value));
    }
}
int main()
{
    unsigned short original;
    __asm { fnstcw original }
    const unsigned short precisions[]={0,0x200,0x300};
    const unsigned exponents[]={0,1,16383,16382,16257,16256,16234,15361,15360,15309,15308,16510,16511,17406,17407,32766,32767};
    const std::uint64_t significands[]={0,1,0x7fffffffffffffffull,0x8000000000000000ull,
        0x8000000000000400ull,0x8000000000000401ull,0x80000000000003ffull,
        0x8000008000000000ull,0x8000010000000000ull,0xffffffffffffffffull,
        0xc000000000000000ull,0x8000000000000001ull};
    const std::uint64_t doubles[]={0,0x8000000000000000ull,1,0x000fffffffffffffull,
        0x0010000000000000ull,0x3ff0000000000000ull,0x7fefffffffffffffull,0x7ff0000000000000ull,
        0xfff0000000000000ull,0x7ff0000000000001ull,0x7ff8abcdef123456ull,0xfff8abcdef123456ull};
    unsigned cases=0,failures=0;
    std::uint32_t random=0x85a721;
    for (unsigned short pc:precisions) for (unsigned short rc=0;rc<0x1000;rc+=0x400)
        for (unsigned sample=0;sample<4096;++sample) {
        random^=random<<13;random^=random>>17;random^=random<<5;
        const std::uint64_t bits=sample<12 ? doubles[sample] :
            (static_cast<std::uint64_t>(random)<<32)|(random^0x8c4129u);
        double input;std::memcpy(&input,&bits,sizeof(bits));
        const unsigned short control=static_cast<unsigned short>((original&~0xf00)|pc|rc|0x3f);
        __asm { fldcw control }
        unsigned char old[12],now[12];std::memset(old,0xcd,12);std::memset(now,0xcd,12);
        LegacyStore(&input,old+1);CandidateStore(&input,now+1);
        if (std::memcmp(old,now,12)!=0 && failures++<8)
            std::printf("codec store mismatch pc=%x rc=%x sample=%u bits=%llx\n",pc,rc,sample,bits);
        ++cases;
        std::uint64_t significand;
        unsigned exponent,sign;
        if (sample<408) {
            significand=significands[sample%12];exponent=exponents[(sample/12)%17];sign=sample/204;
        } else {
            random^=random<<13;random^=random>>17;random^=random<<5;
            significand=(static_cast<std::uint64_t>(random)<<32)|(random^0x7fa834u);
            exponent=(random>>8)&0x7fff;sign=random>>31;
        }
        unsigned char extended[10];
        for (unsigned index=0;index<8;++index) extended[index]=static_cast<unsigned char>(significand>>(index*8));
        extended[8]=static_cast<unsigned char>(exponent);
        extended[9]=static_cast<unsigned char>((exponent>>8)|(sign<<7));
        for (unsigned single=0;single<2;++single) {
            std::uint64_t a=UINT64_C(0x1234567887654321),b=a;
            LegacyLoad(extended,&a,single!=0);CandidateLoad(extended,&b,single!=0);
            if (a!=b && failures++<8)
                std::printf("codec load mismatch pc=%x rc=%x sample=%u single=%u exp=%x sig=%llx old=%llx new=%llx\n",
                    pc,rc,sample,single,exponent,significand,a,b);
            ++cases;
        }
    }
    __asm { fnclex }
    __asm { fldcw original }
    std::printf("%u exact transport conversions; %u failures\n",cases,failures);
    return failures?1:0;
}
