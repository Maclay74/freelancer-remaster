#include "../../src/Thorn/SceneInterpolationMath.h"
#include "../../src/Thorn/CompactLightMath.h"
#include "../../src/Thorn/PortableSplineMath.h"
#include <cstdio>
#include <limits>

struct FThornEntity;
struct IPSysControl {
    FThornEntity* Owner;float Argument,Observed;unsigned Calls;
    void SetSparam(float value);
};
struct FThornEntity { float Descriptor,PsysRuntimeSparam;IPSysControl* PsysControl; };
void IPSysControl::SetSparam(float value)
{
    Argument=value;Observed=Owner->PsysRuntimeSparam;++Calls;
}
static void InitializeEventFrameWords(float* frame)
{
    const float identity[9]={1,0,0,0,1,0,0,0,1};std::memcpy(frame,identity,sizeof(identity));
}
#include "candidates/legacy_scene_interpolation.h"

__declspec(noinline) static bool Run(bool legacy,unsigned op,float* values,unsigned output)
{
    float* from=values+4;float* to=values+11;float* progress=values+16;float* out=values+output;
    if (legacy) {
        switch(op) {
        case 0: LegacySceneInterpolation::FogLerpFloat(from,to,progress,out);break;
        case 1: LegacySceneInterpolation::InterpolateFloorHeight(from,to,progress,out);break;
        case 2: LegacySceneInterpolation::InterpolateLightDirection(out,from,to,progress);break;
        case 3: LegacySceneInterpolation::LerpSpatialPosition(out,from,to,progress);break;
        case 4: LegacySceneInterpolation::AccumulateAxisDelta(progress,to,from,out);break;
        case 5: return LegacySceneInterpolation::BuildAttachLookDirection(out,to,from);
        case 6: LegacySceneInterpolation::ParticleLerp(from,*to,*progress,out);break;
        case 7: LegacySceneInterpolation::NegatePosition(out,from);break;
        }
    } else {
        switch(op) {
        case 0: case 1: ThornSceneInterpolation::Lerp(from,to,progress,out);break;
        case 2: ThornSceneInterpolation::LightDirection(out,from,to,progress);break;
        case 3: ThornSceneInterpolation::SpatialPosition(out,from,to,progress);break;
        case 4: ThornSceneInterpolation::AxisDelta(progress,to,from,out);break;
        case 5: return ThornSceneInterpolation::AttachDirection(out,to,from);
        case 6: {
            const float target=*to,scale=*progress;
            ThornSceneInterpolation::Lerp(from,&target,&scale,out);break;
        }
        case 7: ThornPortableSplineMath::NegateStored(out,from);break;
        }
    }
    return false;
}
__declspec(noinline) static void Rotation(bool legacy,float* output,int axis,const float* degrees)
{
    if (legacy) LegacySceneInterpolation::BuildAxisRotation(output,axis,degrees);
    else ThornSceneInterpolation::AxisRotation(output,axis,degrees);
}
__declspec(noinline) static void Compact(bool legacy,const unsigned char* source,unsigned char* target)
{
    if (legacy) LegacySceneInterpolation::ConvertCompactLightRetail(source,target);
    else ThornCompactLight::Convert(source,target);
}
__declspec(noinline) static void Particle(FThornEntity* entity,const float* value)
{
    const float argument=ThornSceneInterpolation::PublishParticle(&entity->PsysRuntimeSparam,value);
    auto* control=entity->PsysControl;
    if (control) control->SetSparam(argument);
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
    const float special[]={0,-0.0f,1,-1,std::numeric_limits<float>::denorm_min(),inf,nan};
    const unsigned outputs[]={2,4,5,10,11,12,16,17};
    const int axes[]={-1,0,1,2,3,4,5,6,100};
    const float angles[]={0,-0.0f,45,90,180,270,360,-90,720,inf,nan};
    const unsigned targets[]={0,4,28,32,60,61,64,96,124,128,132};
    const float ranges[]={0,-0.0f,0.00001f,0.0001f,1,1000,-1,nan,inf};
    const float cutoffs[]={-1,-0.0f,0,45,90,180,360,nan,inf};
    std::uint32_t random=0x58b129;
    unsigned cases=0,failures=0;
    for (unsigned short pc:precisions) for (unsigned short rc=0;rc<0x1000;rc+=0x400)
        for (unsigned sample=0;sample<2048;++sample) {
        __asm { fldcw measurement }
        float seed[24];
        for (float& value:seed) {random^=random<<13;random^=random>>17;random^=random<<5;value=(static_cast<int>(random%4097)-2048)/1024.0f;}
        if (sample<7) for (float& value:seed) value=special[sample];
        const unsigned output=outputs[sample%8];
        const unsigned short control=static_cast<unsigned short>((original&~0xf00)|pc|rc|0x3f);
        for (unsigned op=0;op<8;++op) {
            float old[24],now[24];std::memcpy(old,seed,sizeof(seed));std::memcpy(now,seed,sizeof(seed));
            __asm { fldcw control }
            const bool a=Run(true,op,old,output),b=Run(false,op,now,output);
            __asm { fldcw measurement }
            bool good=a==b;
            const unsigned count=op==2 || op==3 || op==5 || op==7 ? 3:1;
            for (unsigned index=0;index<24;++index) {
                good &= Close(old[index],now[index]);
                const bool written=(index>=output && index<output+count) || (op==4 && index==4);
                if (!written || op==7) good &= std::memcmp(old+index,now+index,4)==0;
            }
            if (!good && failures++<8) std::printf("interpolation mismatch pc=%x rc=%x op=%u sample=%u alias=%u\n",pc,rc,op,sample,output);
            ++cases;
        }
        {
            const int axis=axes[sample%9];
            const float degrees=sample<99 ? angles[sample/9] : seed[0]*360.0f;
            const float* input=axis<0 || axis>5 ? nullptr:&degrees;
            float old[9],now[9];
            __asm { fldcw control }
            Rotation(true,old,axis,input);Rotation(false,now,axis,input);
            __asm { fldcw measurement }
            bool good=true;for (unsigned index=0;index<9;++index) good &= Close(old[index],now[index]);
            if (!good && failures++<8) std::printf("axis rotation mismatch pc=%x rc=%x sample=%u axis=%d\n",pc,rc,sample,axis);
            ++cases;
        }
        {
            FThornEntity old={seed[0],seed[1],nullptr},now=old;
            IPSysControl oldControl={&old,0,0,0},nowControl={&now,0,0,0};
            if (sample%3) {old.PsysControl=&oldControl;now.PsysControl=&nowControl;}
            const float* oldValue=sample&1 ? &old.PsysRuntimeSparam:&old.Descriptor;
            const float* nowValue=sample&1 ? &now.PsysRuntimeSparam:&now.Descriptor;
            __asm { fldcw control }
            LegacySceneInterpolation::ApplyPsysSparam(&old,oldValue);Particle(&now,nowValue);
            __asm { fldcw measurement }
            const bool good=std::memcmp(&old.Descriptor,&now.Descriptor,4)==0 &&
                std::memcmp(&old.PsysRuntimeSparam,&now.PsysRuntimeSparam,4)==0 && oldControl.Calls==nowControl.Calls &&
                std::memcmp(&oldControl.Argument,&nowControl.Argument,4)==0 && std::memcmp(&oldControl.Observed,&nowControl.Observed,4)==0;
            if (!good && failures++<8) std::printf("particle mismatch pc=%x rc=%x sample=%u\n",pc,rc,sample);
            ++cases;
        }
        {
            unsigned char old[256],now[256];
            for (auto& byte:old) {random^=random<<13;random^=random>>17;random^=random<<5;byte=static_cast<unsigned char>(random);}
            const unsigned source=sample%3 ? 32:96,target=targets[sample%11];
            for (unsigned index=0;index<3;++index) {
                const std::int32_t color=static_cast<std::int32_t>(random%1025)-255;
                std::memcpy(old+source+4*index,&color,4);random^=random<<13;random^=random>>17;random^=random<<5;
            }
            const float range=ranges[sample%9],cutoff=cutoffs[(sample/9)%9];
            const std::int32_t infinite=sample&1;
            std::memcpy(old+source+0x18,&range,4);std::memcpy(old+source+0x1c,&cutoff,4);
            std::memcpy(old+source+0x24,&infinite,4);std::memcpy(now,old,sizeof(old));
            __asm { fldcw control }
            Compact(true,old+source,old+target);Compact(false,now+source,now+target);
            __asm { fldcw measurement }
            bool good=true,raw[256];for (bool& value:raw) value=true;
            const unsigned numeric[]={4,8,12,0x4c,0x5c,0x60,0x64};
            for (unsigned offset:numeric) {
                const float a=ThornCompactLight::Read<float>(old+target,offset),b=ThornCompactLight::Read<float>(now+target,offset);
                good &= Close(a,b);
                for (unsigned byte=0;byte<4;++byte) raw[target+offset+byte]=false;
            }
            for (unsigned index=0;index<256;++index) if (raw[index]) good &= old[index]==now[index];
            if (!good && failures++<8) std::printf("compact light mismatch pc=%x rc=%x sample=%u source=%u target=%u\n",pc,rc,sample,source,target);
            ++cases;
        }
    }
    __asm { fnclex }
    __asm { fldcw original }
    std::printf("%u scene interpolation/publication cases; %u failures\n",cases,failures);
    return failures?1:0;
}
