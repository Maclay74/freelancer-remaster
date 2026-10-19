#include "../../src/Thorn/SceneGeometryMath.h"
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <limits>
struct FVec3 { float X, Y, Z; };
#include "candidates/legacy_scene_geometry.h"

static bool Close(float a, float b, bool normalized)
{
    if (std::isnan(a)) return std::isnan(b);
    if (std::isinf(a)) return a == b;
    return std::fabs(static_cast<double>(a)-b) <= 2e-6*(normalized ? 1 : 1+std::fabs(static_cast<double>(a)));
}

__declspec(noinline) static bool Run(bool legacy, unsigned op, float* buffer, unsigned output)
{
    float* a = buffer+4; float* b = buffer+12; float* dst = buffer+output;
    if (legacy) {
        switch(op) {
        case 0: LegacySceneGeometry::Cross3(dst,a,b); break;
        case 1: LegacySceneGeometry::CrossAxisSide3(dst,a,b,false); break;
        case 2: LegacySceneGeometry::CrossAxisSide3(dst,a,b,true); break;
        case 3: LegacySceneGeometry::CrossAxisUp3(dst,a,b); break;
        case 4: LegacySceneGeometry::Normalize3(a); break;
        case 5: LegacySceneGeometry::NormalizeAxis3(a); break;
        default: return LegacySceneGeometry::AxisDotNotGreater(a,b);
        }
    } else {
        switch(op) {
        case 0: ThornSceneGeometry::Cross(dst,a,b); break;
        case 1: ThornSceneGeometry::CrossSide(dst,a,b,false); break;
        case 2: ThornSceneGeometry::CrossSide(dst,a,b,true); break;
        case 3: ThornSceneGeometry::CrossUp(dst,a,b); break;
        case 4: case 5: ThornSceneGeometry::Normalize(a[0],a[1],a[2]); break;
        default: return ThornSceneGeometry::AxisDotNotGreater(a,b);
        }
    }
    return false;
}

__declspec(noinline) static void Look(bool legacy, unsigned op, FVec3* vectors, unsigned output)
{
    if (legacy) {
        if (op == 0) LegacySceneGeometry::NormalizeLookAtVector(vectors[0]);
        else LegacySceneGeometry::CrossLookAtVector(vectors[output],vectors[0],vectors[1],op == 2);
    } else {
        if (op == 0) ThornSceneGeometry::Normalize(vectors[0].X,vectors[0].Y,vectors[0].Z,true);
        else ThornSceneGeometry::LookAtCross(vectors[output],vectors[0],vectors[1],op == 2);
    }
}

__declspec(noinline) static void Frame(bool legacy, const FVec3& tangent, float* frame)
{
    if (legacy) LegacySceneGeometry::BuildLookAtFromTangent(tangent,frame);
    else ThornSceneGeometry::BuildLookAtFrame(tangent,frame);
}

int main()
{
    unsigned short original;
    __asm { fnstcw original }
    const unsigned short measurement = static_cast<unsigned short>((original & ~0xf00) | 0x200 | 0x3f);
    const unsigned short precisions[] = {0, 0x200, 0x300};
    const unsigned outputs[] = {0,4,5,11,12,13,18};
    const float special[] = {0,-0.0f,std::numeric_limits<float>::denorm_min(),
        std::numeric_limits<float>::infinity(),std::numeric_limits<float>::quiet_NaN(),
        std::numeric_limits<float>::max()};
    std::uint32_t random = 0x5ab413;
    unsigned cases=0, failures=0;
    for (unsigned short pc : precisions) for (unsigned short rc=0; rc<0x1000; rc+=0x400)
        for (unsigned sample=0; sample<1024; ++sample) {
        __asm { fldcw measurement }
        float seed[24];
        for (float& value : seed) {
            random^=random<<13; random^=random>>17; random^=random<<5;
            value=sample<6 ? special[sample] : (static_cast<int>(random%65537)-32768)/1024.0f;
        }
        if (sample>=6 && sample<10) { // Straddle the near-parallel threshold.
            seed[4]=0; seed[5]=1; seed[6]=0;
            seed[12]=0; seed[13]=sample&1 ? 0.99f : std::nextafter(0.99f,0.0f); seed[14]=0;
        }
        const unsigned short control=static_cast<unsigned short>((original&~0xf00)|pc|rc|0x3f);
        for (unsigned op=0; op<7; ++op) {
            float old[24], now[24]; std::memcpy(old,seed,sizeof(seed)); std::memcpy(now,seed,sizeof(seed));
            const unsigned output=outputs[sample%7];
            __asm { fldcw control }
            const bool oldFlag=Run(true,op,old,output);
            const bool nowFlag=Run(false,op,now,output);
            __asm { fldcw measurement }
            bool good=oldFlag==nowFlag;
            const unsigned written=op==4 || op==5 ? 4 : output;
            for (unsigned index=0; index<24; ++index) {
                good &= Close(old[index],now[index],op==4 || op==5);
                if (op==6 || index<written || index>=written+3)
                    good &= std::memcmp(&old[index],&now[index],sizeof(float))==0;
            }
            if (!good && failures++<8)
                std::printf("scene geometry mismatch pc=%x rc=%x op=%u sample=%u output=%u\n",pc,rc,op,sample,output);
            ++cases;
        }
        for (unsigned op=0; op<3; ++op) {
            FVec3 old[3]={{seed[4],seed[5],seed[6]},{seed[12],seed[13],seed[14]},{1,2,3}};
            FVec3 now[3]; std::memcpy(now,old,sizeof(old));
            __asm { fldcw control }
            Look(true,op,old,sample%3); Look(false,op,now,sample%3);
            __asm { fldcw measurement }
            float a[9],b[9]; std::memcpy(a,old,sizeof(a)); std::memcpy(b,now,sizeof(b));
            bool good=true;
            for (unsigned index=0; index<9; ++index) {
                good &= Close(a[index],b[index],op==0);
                if (index/3 != (op==0 ? 0 : sample%3))
                    good &= std::memcmp(&a[index],&b[index],sizeof(float))==0;
            }
            if (!good && failures++<8)
                std::printf("look-at geometry mismatch pc=%x rc=%x op=%u sample=%u output=%u\n",pc,rc,op,sample,sample%3);
            ++cases;
        }
        {
            const FVec3 tangent={seed[4],seed[5],seed[6]};
            float old[12]={1,0,0,0,1,0,0,0,1,17,18,19};
            float now[12]; std::memcpy(now,old,sizeof(old));
            __asm { fldcw control }
            Frame(true,tangent,old); Frame(false,tangent,now);
            __asm { fldcw measurement }
            bool good=true;
            for (unsigned index=0; index<12; ++index) good &= Close(old[index],now[index],true);
            if (!good && failures++<8)
                std::printf("look-at frame mismatch pc=%x rc=%x sample=%u\n",pc,rc,sample);
            ++cases;
        }
    }
    __asm { fnclex }
    __asm { fldcw original }
    std::printf("%u geometry cases; %u failures\n",cases,failures);
    return failures ? 1 : 0;
}
