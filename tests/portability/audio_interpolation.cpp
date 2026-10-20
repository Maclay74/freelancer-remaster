#include "../../src/Thorn/SceneInterpolationMath.h"
#include <cstdio>
#include <algorithm>
__declspec(noinline) static void Reference(float* output,const float* from,const float* to,double progress) {
    __asm {
        fld qword ptr progress
        mov eax,from
        fld dword ptr [eax]
        mov eax,to
        fld dword ptr [eax]
        fsub st(0),st(1)
        fmul st(0),st(2)
        fadd st(0),st(1)
        mov eax,output
        fstp dword ptr [eax]
        fstp st(0)
        fstp st(0)
    }
}
__declspec(noinline) static void Candidate(float* output,const float* from,const float* to,double progress) {
    ThornSceneInterpolation::Audio(output,from,to,progress);
}
int main() {
    unsigned short original;__asm { fnstcw original }
    unsigned random=39117,cases=0;
    for(unsigned mode=0;mode<12;++mode) {
        const unsigned short control=static_cast<unsigned short>((original&~0xf00)|(mode/4==0?0:mode/4==1?0x200:0x300)|((mode%4)<<10)|0x3f);
        __asm { fldcw control }
        for(unsigned sample=0;sample<8192;++sample) for(unsigned output=0;output<3;++output) {
            random^=random<<13;random^=random>>17;random^=random<<5;
            float a[3]={float(random%200001)/32.0f,float((random*1237u)%200001)/64.0f,77},b[3];
            std::memcpy(b,a,sizeof a);
            const double progress=double(sample%401)/100.0-1.0;
            Reference(a+output,a,a+1,progress);Candidate(b+output,b,b+1,progress);
            for(unsigned i=0;i<3;++i) if(std::abs(double(a[i])-b[i])>5e-6*std::max(1.0,std::abs(double(a[i])))) {
                std::printf("FAIL mode=%u sample=%u output=%u\n",mode,sample,output);return 1;
            }
            ++cases;
        }
    }
    __asm { fldcw original }
    std::printf("PASS: %u audio float comparisons across 12 precision/rounding modes and output aliases\n",cases);
    return 0;
}
