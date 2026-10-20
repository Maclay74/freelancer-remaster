#include "../../src/Thorn/SceneInterpolationMath.h"
#include <cstdio>
__declspec(noinline) static double Reference(unsigned elapsed,int duration,bool narrow) {
    const long long wide=elapsed;double result;float single;
    __asm { fild qword ptr wide }
    __asm { fidiv dword ptr duration }
    if(narrow) {
        __asm { fstp dword ptr single }
        return single;
    }
    __asm { fstp qword ptr result }
    return result;
}
__declspec(noinline) static double Candidate(unsigned elapsed,int duration,bool narrow) {
    const double value=ThornSceneInterpolation::WrapIntermediate(static_cast<double>(elapsed)/duration);
    return narrow?static_cast<double>(static_cast<float>(value)):value;
}
int main() {
    unsigned short original;__asm { fnstcw original }
    unsigned random=39117,cases=0;
    for(unsigned mode=0;mode<12;++mode) {
        const unsigned short control=static_cast<unsigned short>((original&~0xf00)|(mode/4==0?0:mode/4==1?0x200:0x300)|((mode%4)<<10)|0x3f);
        __asm { fldcw control }
        for(unsigned sample=0;sample<8192;++sample) {
            random^=random<<13;random^=random>>17;random^=random<<5;
            int duration;std::memcpy(&duration,&random,4);
            if(sample%11==0) duration=0;
            for(unsigned narrow=0;narrow<2;++narrow) {
                const double a=Reference(random,duration,narrow!=0),b=Candidate(random,duration,narrow!=0);
                if(!(std::isnan(a)&&std::isnan(b)) && a!=b &&
                    (!std::isfinite(a)||!std::isfinite(b)||std::abs(a-b)>5e-7*std::abs(a))) {
                    std::printf("FAIL mode=%u elapsed=%u duration=%d narrow=%u %.17g %.17g\n",mode,random,duration,narrow,a,b);return 1;
                }
                ++cases;
            }
        }
    }
    __asm { fldcw original }
    std::printf("PASS: %u event quotient comparisons across 12 precision/rounding modes\n",cases);
    return 0;
}
