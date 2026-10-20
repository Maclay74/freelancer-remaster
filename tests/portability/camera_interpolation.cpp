#include "../../src/Thorn/SceneInterpolationMath.h"
#include "../../src/Thorn/ExtendedProgressCodec.h"
#include <cstdio>
#include <algorithm>
struct FThornExtendedProgress { unsigned char Bytes[10]; };
#include "candidates/legacy_camera_interpolation.h"
int main()
{
    unsigned random=735123, cases=0;
    for (unsigned sample=0;sample<512;++sample)
    for (int mask=0;mask<16;++mask)
    for (unsigned output=0;output<12;++output) {
        float expected[20],actual[20];
        for (float& value : expected) {
            random^=random<<13;random^=random>>17;random^=random<<5;
            value=static_cast<float>(static_cast<int>(random%200001)-100000)/32.0f;
        }
        std::memcpy(actual,expected,sizeof actual);
        const double progress=static_cast<double>(sample%65)/32.0-0.5;
        FThornExtendedProgress encoded;
        ThornExtendedProgress::StoreDouble(encoded.Bytes,progress);
        LegacyCameraInterpolation(expected+output,expected+8,expected+4,&encoded,mask);
        ThornSceneInterpolation::Camera(actual+output,actual+8,actual+4,progress,mask);
        for (unsigned index=0;index<20;++index) {
            const double error=std::abs(static_cast<double>(actual[index])-expected[index]);
            if (error>2e-6*std::max(1.0,std::abs(static_cast<double>(expected[index])))) {
                std::printf("FAIL sample=%u mask=%d output=%u index=%u\n",sample,mask,output,index);return 1;
            }
        }
        ++cases;
    }
    std::printf("PASS: %u camera interpolation comparisons including overlapping arrays and every mask\n",cases);
}
