#include "../../src/Thorn/ParamCurveMath.h"
#include "../../src/Thorn/HermiteRecord.h"
#include <cstdio>
using ThornCurveDetail::FHermiteKnot;
static const float G_MinDeltaT=1e-4f,G_ScalarHalfFloat=0.5f;
#include "candidates/legacy_param_control.h"

__declspec(noinline) static float Normalize(const float* t,const float* a,const float* b)
{return ThornParamMath::NormalizeTime(t,a,b);}
__declspec(noinline) static float Wrap(int a,int b) {return ThornParamMath::WrappedRatio(a,b);}
__declspec(noinline) static void Tangents(FHermiteKnot* values,int count,int mode)
{ThornParamMath::ApplyTangents(values,count,mode);}
__declspec(noinline) static unsigned Fault(bool legacy,int a,int b,float* output)
{
    __try { *output=legacy ? LegacyParamControl::WrappedRatio(a,b):Wrap(a,b);return 0; }
    __except(EXCEPTION_EXECUTE_HANDLER) { return GetExceptionCode(); }
}
__declspec(noinline) static bool Compare(bool legacy,const float* a,const float* b,unsigned operation)
{
    if (!legacy) return operation==0 ? ThornParamMath::LessEqual(a,b):
        operation==1 ? ThornParamMath::Greater(a,b):ThornParamMath::Equal(a,b);
    unsigned char result;
    __asm {
        mov eax,a
        fld dword ptr [eax]
        mov eax,b
        fcomp dword ptr [eax]
        fnstsw ax
        cmp operation,0
        je ParamLessEqual
        cmp operation,1
        je ParamGreater
        test ah,44h
        setnp result
        jmp ParamCompareDone
    ParamLessEqual:
        test ah,41h
        setnp result
        jmp ParamCompareDone
    ParamGreater:
        test ah,41h
        setz result
    ParamCompareDone:
    }
    return result!=0;
}
static bool Close(float a,float b)
{
    if (std::isnan(a)) return std::isnan(b);
    if (std::isinf(a)) return a==b;
    return std::fabs(static_cast<double>(a)-b)<=3e-6*(1+std::fabs(static_cast<double>(a)));
}
int main()
{
    unsigned short original;
    __asm { fnstcw original }
    const unsigned short measurement=static_cast<unsigned short>((original&~0xf00)|0x200|0x3f);
    const unsigned short precisions[]={0,0x200,0x300};
    const float inf=std::numeric_limits<float>::infinity(),nan=std::numeric_limits<float>::quiet_NaN();
    const float times[]={0,-0.0f,1,-1,0.5f,inf,-inf,nan,std::numeric_limits<float>::denorm_min()};
    unsigned cases=0,failures=0;
    std::uint32_t random=0x4b71a9;
    for (unsigned short pc:precisions) for (unsigned short rc=0;rc<0x1000;rc+=0x400)
        for (unsigned sample=0;sample<2048;++sample) {
        __asm { fldcw measurement }
        random^=random<<13;random^=random>>17;random^=random<<5;
        const float t=sample<9 ? times[sample]:(static_cast<int>(random%8193)-4096)/1024.0f;
        const float a=times[sample%9],b=times[(sample/9)%9];
        const int id=static_cast<int>(sample%5)-1;
        const int numerator=static_cast<int>(random);
        const int magnitude=static_cast<int>((random>>1)|1u);
        const int divisor=sample&1 ? -magnitude:magnitude;
        FHermiteKnot old[8],now[8];
        for (unsigned index=0;index<8;++index) {
            const float time=sample%4==0 ? index*0.125f:sample%4==1 ? (8-index)*0.125f:
                sample%4==2 ? 0.0f:index*0.000001f;
            random^=random<<13;random^=random>>17;random^=random<<5;
            old[index]={sample%97==0 ? nan:(static_cast<int>(random%8193)-4096)/1024.0f,time,17.0f,19.0f};
        }
        std::memcpy(now,old,sizeof(old));
        const unsigned short control=static_cast<unsigned short>((original&~0xf00)|pc|rc|0x3f);
        __asm { fldcw control }
        const int oldClass=LegacyParamControl::ClassifyTime(t),newClass=ThornParamMath::ClassifyTime(t);
        float oldT=99,newT=99;
        const bool oldAccept=LegacyParamControl::ReplacementTime(id,t,&oldT),newAccept=ThornParamMath::ReplacementTime(id,t,&newT);
        const float oldU=LegacyParamControl::NormalizeTime(&t,&a,&b),newU=Normalize(&t,&a,&b);
        const float oldWrap=LegacyParamControl::WrappedRatio(numerator,divisor),newWrap=Wrap(numerator,divisor);
        const int count=(sample/9)%9,mode=static_cast<int>(sample%9)-1;
        LegacyParamControl::ApplyHermiteTangents(old,count,mode);Tangents(now,count,mode);
        bool comparisons=true;
        for (unsigned operation=0;operation<3;++operation) comparisons &= Compare(true,&a,&b,operation)==Compare(false,&a,&b,operation);
        __asm { fldcw measurement }
        bool good=oldClass==newClass && oldAccept==newAccept && Close(oldT,newT) && Close(oldU,newU) && Close(oldWrap,newWrap) && comparisons;
        for (unsigned index=0;index<8;++index) {
            good &= std::memcmp(&old[index].Value,&now[index].Value,8)==0;
            good &= Close(old[index].MIn,now[index].MIn) && Close(old[index].MOut,now[index].MOut);
            if (mode==0 && index+1<static_cast<unsigned>(count)) good &= std::memcmp(&old[index].MOut,&now[index].MOut,4)==0;
        }
        if (!good && failures++<8)
            std::printf("curve control mismatch pc=%x rc=%x sample=%u class=%d/%d accept=%d/%d time=%g/%g U=%g/%g wrap=%g/%g mode=%d count=%d\n",
                pc,rc,sample,oldClass,newClass,oldAccept,newAccept,oldT,newT,oldU,newU,oldWrap,newWrap,mode,count);
        ++cases;
    }
    const int divisors[]={0,-1};
    for (int divisor:divisors) {
        const int numerator=divisor ? (-2147483647-1):1;
        float a=-100,b=-100;
        const unsigned old=Fault(true,numerator,divisor,&a),now=Fault(false,numerator,divisor,&b);
        if (!old || old!=now || a!=b) {
            std::printf("wrap fault mismatch divisor=%d old=%x new=%x output=%g/%g\n",divisor,old,now,a,b);
            ++failures;
        }
        ++cases;
    }
    __asm { fnclex }
    __asm { fldcw original }
    std::printf("%u curve-control/tangent/fault cases; %u failures\n",cases,failures);
    return failures?1:0;
}
