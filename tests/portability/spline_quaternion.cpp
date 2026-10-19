#include "../../src/Thorn/SplineQuaternionMath.h"
#include "../../src/Thorn/SplineMath.h"
#include <cstdint>
#include <cstdio>
#include <limits>
struct FQuat { float W,X,Y,Z; };
struct FVec3 { float X,Y,Z; };
static const float G_QuatSinEps=0.001f;
#include "candidates/legacy_spline_quaternion.h"

struct State { FQuat values[3]; float scalar; };
static_assert(sizeof(State)==13*sizeof(float),"Packed fixture");
__declspec(noinline) static void Run(bool legacy,unsigned op,State& state,float t,unsigned output)
{
    auto& dst=state.values[output]; const auto& a=state.values[0];const auto& b=state.values[1];
    if (legacy) {
        switch(op) {
        case 0: LegacySplineQuaternion::QuatInverseScaleRetail(a,state.scalar);break;
        case 1: state.scalar=LegacySplineQuaternion::SquadBlendRetail(&t);break;
        case 2: LegacySplineQuaternion::QuatAdd(dst,a,b);break;
        case 3: LegacySplineQuaternion::QuatScale(dst,a,t);break;
        case 4: case 5: LegacySplineQuaternion::QuatLogRetail(dst,a,op==5);break;
        case 6: LegacySplineQuaternion::QuatFirstControlExp(dst,a);break;
        case 7: case 8: LegacySplineQuaternion::QuatExpRetail(dst,a,op==8);break;
        }
    } else {
        switch(op) {
        case 0: state.scalar=ThornSplineQuaternion::InverseScale(a);break;
        case 1: state.scalar=ThornSplineQuaternion::SquadBlend(t);break;
        case 2: ThornSplineQuaternion::Add(dst,a,b);break;
        case 3: ThornSplineQuaternion::Scale(dst,a,t);break;
        case 4: case 5: ThornSplineQuaternion::Log(dst,a,op==5,G_QuatSinEps);break;
        case 6: ThornSplineQuaternion::Exp(dst,a,true,true,G_QuatSinEps);break;
        case 7: case 8: ThornSplineQuaternion::Exp(dst,a,op==8,false,G_QuatSinEps);break;
        }
    }
}
static bool Close(float a,float b)
{
    if (std::isnan(a)) return std::isnan(b);
    if (std::isinf(a)) return a==b;
    return std::fabs(static_cast<double>(a)-b)<=5e-6*(1+std::fabs(static_cast<double>(a)));
}
int main()
{
    unsigned short original;
    __asm { fnstcw original }
    const unsigned short measurement=static_cast<unsigned short>((original&~0xf00)|0x200|0x3f);
    const unsigned short precisions[]={0,0x200,0x300};
    const float nan=std::numeric_limits<float>::quiet_NaN(),inf=std::numeric_limits<float>::infinity();
    const FQuat special[]={{0,0,0,0},{1,0,0,0},{-1,0,0,0},{0,1,0,0},{0,0.0005f,0,0},
        {0,0.001f,0,0},{0,0.001001f,0,0},{0.99999994f,1,2,3},{-0.99999994f,1,2,3},
        {nan,1,2,3},{inf,1,2,3},{0,nan,2,3},{0,inf,2,3},
        {0,std::numeric_limits<float>::denorm_min(),0,0},{0,3.1415927f,0,0},{0,6.2831855f,0,0}};
    std::uint32_t random=0x7ab316;
    unsigned cases=0,failures=0;
    for (unsigned short pc:precisions) for (unsigned short rc=0;rc<0x1000;rc+=0x400)
        for (unsigned sample=0;sample<2048;++sample) {
        __asm { fldcw measurement }
        State seed;
        float words[13];
        for (float& value:words) { random^=random<<13;random^=random>>17;random^=random<<5;value=(static_cast<int>(random%8193)-4096)/1024.0f; }
        std::memcpy(&seed,words,sizeof(seed));
        seed.values[0].W/=4;
        if (sample<16) seed.values[0]=special[sample];
        const float t=words[12]/2;
        const unsigned output=sample%3;
        const unsigned short control=static_cast<unsigned short>((original&~0xf00)|pc|rc|0x3f);
        for (unsigned op=0;op<9;++op) {
            State old,now;std::memcpy(&old,&seed,sizeof(seed));std::memcpy(&now,&seed,sizeof(seed));
            __asm { fldcw control }
            Run(true,op,old,t,output);Run(false,op,now,t,output);
            __asm { fldcw measurement }
            float a[13],b[13];std::memcpy(a,&old,sizeof(a));std::memcpy(b,&now,sizeof(b));
            bool good=true;
            for (unsigned index=0;index<13;++index) {
                good &= Close(a[index],b[index]);
                const bool written=op<2 ? index==12 : index/4==output;
                if (!written) good &= std::memcmp(a+index,b+index,4)==0;
            }
            if (!good && failures++<10) {
                std::printf("quaternion mismatch pc=%x rc=%x op=%u sample=%u alias=%u\n",pc,rc,op,sample,output);
                for (unsigned index=0;index<13;++index)
                    if (!Close(a[index],b[index])) std::printf("  lane=%u old=%.9g new=%.9g\n",index,a[index],b[index]);
            }
            ++cases;
        }
    }
    __asm { fnclex }
    __asm { fldcw original }
    std::printf("%u quaternion cases; %u failures\n",cases,failures);
    return failures?1:0;
}
