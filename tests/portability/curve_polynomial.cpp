#include "../../src/Thorn/CurvePolynomialMath.h"
#include "../../src/Thorn/ExtendedProgressCodec.h"
#include "../../src/Thorn/HermiteRecord.h"
#include <cstdio>
#include <cmath>
#include <limits>
using ThornCurveDetail::FHermiteKnot;
struct FThornExtendedProgress { unsigned char Bytes[10]; };
struct FParamKey { float T,Value,Extra0,Extra1;int Id; };
static const float G_ScalarOne=1,G_ScalarThree=3,G_ScalarFour=4,G_ScalarFive=5;
static const double G_ScalarHalf=0.5;
#include "candidates/legacy_curve_polynomial.h"

__declspec(noinline) static void Candidate(unsigned op,const float* values,float u,const FParamKey& left,
    const FParamKey& right,const FHermiteKnot* knots,int count,const float* time,FThornExtendedProgress* output)
{
    double result=0;
    switch(op) {
    case 0: result=ThornCurvePolynomial::Catmull(count&1 ? nullptr:values,values+1,values+2,count&2 ? nullptr:values+3,u);break;
    case 1: result=ThornCurvePolynomial::Lerp(left,right,time);break;
    case 2: result=ThornCurvePolynomial::Hermite(knots[0],knots[1],time);break;
    default: result=ThornCurvePolynomial::EvaluateHermite(knots,count,time);break;
    }
    ThornExtendedProgress::StoreDouble(output->Bytes,result);
}
int main()
{
    unsigned short original;
    __asm { fnstcw original }
    const unsigned short measurement=static_cast<unsigned short>((original&~0xf00)|0x200|0x3f);
    const unsigned short precisions[]={0,0x200,0x300};
    unsigned cases=0,failures=0;
    std::uint32_t random=0x3148b7;
    for (unsigned short pc:precisions) for (unsigned short rc=0;rc<0x1000;rc+=0x400)
        for (unsigned sample=0;sample<2048;++sample) {
        __asm { fldcw measurement }
        float values[4];
        for (float& value:values) {random^=random<<13;random^=random>>17;random^=random<<5;value=(static_cast<int>(random%8193)-4096)/512.0f;}
        const int count=2+sample%7;
        const float u=(static_cast<int>(sample%1025)-128)/768.0f;
        float time=sample%17==0 ? 0:sample%17==1 ? 1:u;
        FParamKey left={0,values[1],0,0,0},right={1,values[2],0,0,1};
        FHermiteKnot knots[8];
        for (unsigned index=0;index<8;++index) {
            knots[index]={values[index%4],static_cast<float>(index)/(count-1),values[(index+1)%4],values[(index+2)%4]};
            if (sample%5==0 && index>0 && index<static_cast<unsigned>(count-1)) knots[index].T=knots[index-1].T;
        }
        if (sample%7==0) {const std::uint32_t sentinel=0xffffffffu;std::memcpy(&knots[0].MOut,&sentinel,4);}
        if (sample%17==2) time=knots[1].T;
        const unsigned short control=static_cast<unsigned short>((original&~0xf00)|pc|rc|0x3f);
        for (unsigned op=0;op<4;++op) {
            FThornExtendedProgress old,now;
            __asm { fldcw control }
            switch(op) {
            case 0: LegacyCurvePolynomial::CatmullRom1D(count&1 ? nullptr:values,values+1,values+2,count&2 ? nullptr:values+3,u,&old);break;
            case 1: LegacyCurvePolynomial::ParamLerp(&left,&right,&time,&old);break;
            case 2: LegacyCurvePolynomial::Hermite1D(knots[0],knots[1],&time,&old);break;
            case 3: LegacyCurvePolynomial::HermiteEvaluate(knots,count,&time,&old);break;
            }
            Candidate(op,values,u,left,right,knots,count,&time,&now);
            const double a=ThornExtendedProgress::LoadDouble(old.Bytes),b=ThornExtendedProgress::LoadDouble(now.Bytes);
            __asm { fldcw measurement }
            const double error=std::fabs(a-b)/(1+std::fabs(a));
            const bool good=std::isnan(a) ? std::isnan(b):std::isinf(a) ? a==b:error<=5e-6;
            if (!good && failures++<10)
                std::printf("curve polynomial mismatch pc=%x rc=%x sample=%u op=%u time=%.9g old=%.17g new=%.17g error=%g\n",
                    pc,rc,sample,op,time,a,b,error);
            ++cases;
        }
    }
    __asm { fldcw measurement }
    const FHermiteKnot singleton={3,0,0,0};
    const float nan=std::numeric_limits<float>::quiet_NaN();
    if (ThornCurvePolynomial::EvaluateHermite<FHermiteKnot>(nullptr,0,nullptr)!=0 ||
        ThornCurvePolynomial::EvaluateHermite(&singleton,1,nullptr)!=3 ||
        !std::isnan(ThornCurvePolynomial::Hermite(singleton,FHermiteKnot{5,1,1,1},&nan))) ++failures;
    __asm { fnclex }
    __asm { fldcw original }
    std::printf("%u curve polynomial/walk cases; %u failures\n",cases,failures);
    return failures?1:0;
}
