#include "../../src/Thorn/SceneInterpolationMath.h"
#include <cstdio>
#include <limits>
#include <algorithm>
struct FThornExtendedProgress { unsigned char Bytes[10]; };
struct FThornEvent { float FloorWrapLast; };
#include "candidates/legacy_scene_wrap.h"
static bool Same(float a,float b) {
    if(std::isnan(a)||std::isnan(b)) return std::isnan(a)&&std::isnan(b);
    if(a==b) return true;
    if(!std::isfinite(a)||!std::isfinite(b)) return false;
    return std::abs(double(a)-b)<=5e-6*std::max(1.0,std::abs(double(a)));
}
__declspec(noinline) static void Spatial(float* values,const FThornExtendedProgress* progress,unsigned rounded,unsigned scale) {
    ThornSceneInterpolation::SpatialWrap(values,progress->Bytes,values+rounded,values+scale);
}
__declspec(noinline) static void Floor(FThornEvent* event,const FThornExtendedProgress* progress,float* output) {
    ThornSceneInterpolation::FloorWrap(&event->FloorWrapLast,progress->Bytes,output);
}
int main() {
    const double values[]={0.0,-0.0,1.0,0.5,-0.5,2.0,0.99999999,1.00000001,
        std::numeric_limits<double>::quiet_NaN(),std::numeric_limits<double>::infinity(),
        -std::numeric_limits<double>::infinity(),1e-40,-1e-40};
    unsigned cases=0,random=18127;
    unsigned short original;
    __asm { fnstcw original }
    for(unsigned mode=0;mode<12;++mode) {
        const unsigned short precision=mode/4==0?0:mode/4==1?0x200:0x300;
        const unsigned short control=static_cast<unsigned short>((original&~0xf00)|precision|((mode%4)<<10)|0x3f);
        __asm { fldcw control }
    for(unsigned sample=0;sample<4096;++sample) {
        random^=random<<13;random^=random>>17;random^=random<<5;
        double progress=sample<169?values[sample%13]:double(random%400001)/100000.0-1.0;
        float last=sample<169?static_cast<float>(values[sample/13]):float(random%200001)/100000.0f;
        FThornExtendedProgress encoded;ThornExtendedProgress::StoreDouble(encoded.Bytes,progress);
        for(unsigned rounded=0;rounded<3;++rounded) for(unsigned scale=0;scale<3;++scale) {
            float a[3]={last,77,88},b[3]={last,77,88};
            LegacySpatialWrapScale(a,&encoded,a+rounded,a+scale);
            Spatial(b,&encoded,rounded,scale);
            for(unsigned i=0;i<3;++i) if(!Same(a[i],b[i])) {
                std::printf("FAIL spatial sample=%u rounded=%u scale=%u index=%u %.9g %.9g\n",sample,rounded,scale,i,a[i],b[i]); std::printf("progress=%.17g last=%.17g pc=%u\n",progress,double(last),ThornFpEnvironment::Precision());return 1;
            }
            ++cases;
        }
        for(unsigned alias=0;alias<2;++alias) {
            FThornEvent a={last},b={last};float x=77,y=77;
            LegacyFloorWrapScale(&a,&encoded,alias?&a.FloorWrapLast:&x);
            Floor(&b,&encoded,alias?&b.FloorWrapLast:&y);
            if(!Same(a.FloorWrapLast,b.FloorWrapLast)||!Same(x,y)) {std::printf("FAIL floor %u\n",sample);return 1;}
            ++cases;
        }
    }
    }
    __asm { fldcw original }
    std::printf("PASS: %u wrap comparisons including output aliases, boundary values, NaN and infinities\n",cases);
    return 0;
}
