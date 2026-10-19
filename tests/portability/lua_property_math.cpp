#include "../../src/Thorn/LuaPropertyMath.h"
#include <cmath>
#include <cstdio>
#include <limits>

using lua_Object=unsigned;
static double inputNumber;
static bool nilObject;
static unsigned trace;
static double lua_getnumber(lua_Object) { trace=(trace<<4)|1; return inputNumber; }
static int lua_isnil(lua_Object) { trace=(trace<<4)|2; return nilObject; }
static constexpr int kThornLightBitColor=1, kThornLightBitInfinite=0x20,
    kThornLightBitCutoff=8, kThornLightBitType=0x100, kThornLightBitDiffuse=0x800;
struct FThornLightProps {
    int Color[3]; float Direction[3]; float RangeTemp,CutoffTemp; int On,Infinite,Type;
    float Diffuse[4],Specular[4],Ambient[4]; int Unused17[6]; float Range;
    int Unused1e; float Atten[3],Theta,Cutoff;
};
static_assert(sizeof(FThornLightProps)==0x90,"Match production light properties");
#include "candidates/legacy_lua_properties.h"

__declspec(noinline) static int CandidateTime(bool start)
{
    if (start) {
        const bool nil=lua_isnil(1)!=0;
        const double value=nil ? 0 : lua_getnumber(1);
        return ThornLuaPropertyMath::Milliseconds(value);
    }
    const double value=lua_getnumber(1);
    return ThornLuaPropertyMath::Milliseconds(value);
}
__declspec(noinline) static void CandidateLight(FThornLightProps* props,int* mask)
{
    ThornLuaPropertyMath::ApplyLight(*props,*mask,kThornLightBitColor,kThornLightBitInfinite,
        kThornLightBitCutoff,kThornLightBitType,kThornLightBitDiffuse);
}
static int* MaskAddress(FThornLightProps& props,int& external,unsigned index)
{
    switch(index) {
    case 0: return props.Color;
    case 1: return props.Color+1;
    case 2: return props.Color+2;
    case 3: return &props.Infinite;
    case 4: return &props.Type;
    default: return &external;
    }
}
int main()
{
    unsigned short original;
    __asm { fnstcw original }
    const unsigned short measurement=static_cast<unsigned short>((original&~0xf00)|0x200|0x3f);
    const unsigned short precisions[]={0,0x200,0x300};
    const double boundaries[]={0,-0.0,1,-1,0.001,-0.001,1.999,-1.999,2147483.647,
        -2147483.648,9223372036854776.0,-9223372036854776.0,
        std::numeric_limits<double>::denorm_min(),std::numeric_limits<double>::max(),
        std::numeric_limits<double>::infinity(),std::numeric_limits<double>::quiet_NaN()};
    const float cutoffs[]={0,-0.0f,-1,1,-std::numeric_limits<float>::denorm_min(),
        -std::numeric_limits<float>::infinity(),std::numeric_limits<float>::quiet_NaN()};
    std::uint32_t random=0x2bb197;
    unsigned times=0,lights=0,failures=0;
    for (unsigned short pc:precisions) for (unsigned short rc=0;rc<0x1000;rc+=0x400)
        for (unsigned sample=0;sample<4096;++sample) {
        __asm { fldcw measurement }
        random^=random<<13; random^=random>>17; random^=random<<5;
        const std::uint64_t raw=(static_cast<std::uint64_t>(random)<<32)|(random^0xc84512u);
        if (sample<16) inputNumber=boundaries[sample];
        else if (sample<1024) {
            const double center=(static_cast<int>(random%200000001)-100000000)/1000.0;
            inputNumber=sample&1 ? std::nextafter(center,std::numeric_limits<double>::infinity()) :
                std::nextafter(center,-std::numeric_limits<double>::infinity());
        } else std::memcpy(&inputNumber,&raw,sizeof(raw));
        nilObject=sample%3==0;
        const unsigned short control=static_cast<unsigned short>((original&~0xf00)|pc|rc|0x3f);
        for (unsigned start=0;start<2;++start) {
            __asm { fldcw control }
            trace=0;
            const int old=start ? LegacyLuaProperties::ReadSceneStartMilliseconds(1) :
                LegacyLuaProperties::LuaMillisecondsToRetailInt(1);
            const unsigned oldTrace=trace;
            trace=0;
            const int now=CandidateTime(start);
            unsigned short after;
            __asm { fnstcw after }
            __asm { fldcw measurement }
            if ((old!=now || oldTrace!=trace || after!=control) && failures++<8)
                std::printf("time mismatch pc=%x rc=%x sample=%u start=%d number=%.17g old=%d new=%d\n",
                    pc,rc,sample,start,inputNumber,old,now);
            ++times;
        }
        FThornLightProps old,now;
        std::uint32_t words[36];
        for (auto& word:words) { random^=random<<13;random^=random>>17;random^=random<<5;word=random; }
        std::memcpy(&old,words,sizeof(old));
        old.Cutoff=cutoffs[sample%7];
        std::memcpy(&now,&old,sizeof(old));
        int oldMask=sample%64,nowMask=oldMask;
        int* oldAddress=MaskAddress(old,oldMask,sample%6);
        int* nowAddress=MaskAddress(now,nowMask,sample%6);
        *oldAddress=*nowAddress=oldMask;
        __asm { fldcw control }
        LegacyLuaProperties::ApplyLightEventPost(&old,oldAddress);
        CandidateLight(&now,nowAddress);
        __asm { fldcw measurement }
        bool good=oldMask==nowMask;
        std::uint32_t a[36],b[36];std::memcpy(a,&old,sizeof(old));std::memcpy(b,&now,sizeof(now));
        for (unsigned index=0;index<36;++index) {
            if (index>=11 && index<=13) {
                float x,y;std::memcpy(&x,a+index,4);std::memcpy(&y,b+index,4);
                good &= std::isnan(x) ? std::isnan(y) : std::isinf(x) ? x==y :
                    std::fabs(static_cast<double>(x)-y)<=2e-6*(1+std::fabs(static_cast<double>(x)));
            } else good &= a[index]==b[index];
        }
        if (!good && failures++<8)
            std::printf("light mismatch pc=%x rc=%x sample=%u alias=%u\n",pc,rc,sample,sample%6);
        ++lights;
    }
    __asm { fnclex }
    __asm { fldcw original }
    std::printf("%u exact time cases; %u light cases; %u failures\n",times,lights,failures);
    return failures ? 1:0;
}
