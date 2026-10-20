#include "../../src/Thorn/SceneIntegerMath.h"
#include <cstdio>
__declspec(noinline) static int Reference(int from,int to,double progress,bool audio) {
    const unsigned difference=static_cast<unsigned>(to)-static_cast<unsigned>(from);
    const unsigned long long wide=difference;
    const float single=static_cast<float>(progress);
    long long result;unsigned short oldControl,chop;
    if(audio) {
        __asm { fild qword ptr wide }
        __asm { fmul qword ptr progress }
    } else {
        __asm { fild dword ptr difference }
        __asm { fmul dword ptr single }
    }
    __asm {
        fiadd dword ptr from
        fnstcw oldControl
        mov ax,oldControl
        or ax,0c00h
        mov chop,ax
        fldcw chop
        fistp qword ptr result
        fldcw oldControl
    }
    return static_cast<int>(static_cast<unsigned>(result));
}
__declspec(noinline) static int Candidate(int from,int to,double progress,bool audio) {
    const float single=static_cast<float>(progress);
    return audio?ThornSceneInteger::Audio(from,to,progress):ThornSceneInteger::Color(from,to,&single);
}
int main() {
    unsigned short original;__asm { fnstcw original }
    unsigned random=197113,cases=0;
    const int endpoints[]={0,1,-1,2147483647,(-2147483647-1),16777217,-16777217};
    for(unsigned mode=0;mode<12;++mode) {
        const unsigned short control=static_cast<unsigned short>((original&~0xf00)|(mode/4==0?0:mode/4==1?0x200:0x300)|((mode%4)<<10)|0x3f);
        __asm { fldcw control }
        for(unsigned sample=0;sample<8192;++sample) {
            random^=random<<13;random^=random>>17;random^=random<<5;
            int from,to;std::memcpy(&from,&random,4);
            unsigned next=random*1664525u+1013904223u;std::memcpy(&to,&next,4);
            if(sample<49){from=endpoints[sample%7];to=endpoints[sample/7];}
            double progress=sample%5==0 ? 1e-30 : sample%5==1 ? -1e-30 :
                sample%5==2 ? double(static_cast<int>(random%400001)-200000)/100000.0 :
                sample%5==3 ? 1.0 : 0.0;
            if(sample%7==0) {
                const std::uint64_t raw=(static_cast<std::uint64_t>(random)<<32)|next;
                std::memcpy(&progress,&raw,8);
            }
            for(unsigned audio=0;audio<2;++audio) {
                const int expected=Reference(from,to,progress,audio!=0);
                const int actual=Candidate(from,to,progress,audio!=0);
                if(expected!=actual) {std::printf("FAIL mode=%u sample=%u audio=%u from=%d to=%d progress=%.17g expected=%d actual=%d\n",mode,sample,audio,from,to,progress,expected,actual);std::printf("precision=%u rounding=%d\n",ThornFpEnvironment::Precision(),ThornFpEnvironment::Rounding());return 1;}
                ++cases;
            }
        }
    }
    __asm { fldcw original }
    std::printf("PASS: %u exact integer interpolation comparisons across 12 precision/rounding modes\n",cases);
    return 0;
}
