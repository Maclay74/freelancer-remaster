#include "../../src/Thorn/SplineSamplingMath.h"
#include <cstdint>
#include <cstdio>
#include <limits>
struct FVec3 { float X,Y,Z; };
static const int G_SamplesPerSegment=17;
#include "candidates/legacy_spline_sampling.h"
struct State { FVec3 vectors[3];float scalars[20];float matrix[9]; };

__declspec(noinline) static void Run(bool legacy,unsigned op,State& s,unsigned variant)
{
    FVec3& previous=s.vectors[0];const FVec3& current=s.vectors[variant==1 ? 0:1];
    float* offset=s.scalars;float* segment=s.scalars+1;float* time=s.scalars+2;
    if (variant==2) segment=offset;
    if (variant==3) time=segment;
    if (variant==4) { segment=&previous.X;offset=&s.vectors[1].Y;time=&previous.Z; }
    if (variant==5) { offset=&previous.Y;time=&s.vectors[1].X; }
    const unsigned output=variant%3;
    if (legacy) {
        switch(op) {
        case 0: LegacySplineSampling::BuildSlot5MatrixRetail(previous,s.matrix);break;
        case 1: LegacySplineSampling::AccumulateSegmentSampleRetail(previous,current,*offset,*segment,*time);break;
        case 2: LegacySplineSampling::NormalizeSegmentSamplesRetail(s.scalars,s.scalars[19]);break;
        case 3: LegacySplineSampling::SubtractGhostVector(s.vectors[output],previous,s.vectors[1]);break;
        case 4: LegacySplineSampling::ExtendEndGhost(previous,s.vectors[output]);break;
        }
    } else {
        switch(op) {
        case 0: ThornSplineSampling::BuildPositionMatrix(previous,s.matrix);break;
        case 1: ThornSplineSampling::Accumulate(previous,current,*offset,*segment,*time);break;
        case 2: ThornSplineSampling::NormalizeSamples(s.scalars,s.scalars[19],17);break;
        case 3: ThornSplineSampling::SubtractGhost(s.vectors[output],previous,s.vectors[1]);break;
        case 4: ThornSplineSampling::ExtendEndGhost(previous,s.vectors[output]);break;
        }
    }
}
static bool Close(float a,float b,double inputScale=0)
{
    if (std::isnan(a)) return std::isnan(b);
    if (std::isinf(a)) return a==b;
    return std::fabs(static_cast<double>(a)-b)<=3e-6*(1+std::fabs(static_cast<double>(a))+inputScale);
}
int main()
{
    static_assert(sizeof(State)==38*sizeof(float),"Packed fixture");
    unsigned short original;
    __asm { fnstcw original }
    const unsigned short measurement=static_cast<unsigned short>((original&~0xf00)|0x200|0x3f);
    const unsigned short precisions[]={0,0x200,0x300};
    const float tiny=std::numeric_limits<float>::denorm_min();
    const float inf=std::numeric_limits<float>::infinity(),nan=std::numeric_limits<float>::quiet_NaN();
    const FVec3 special[]={{0,0,0},{1,0,0},{0,1,0},{0,0,1},{-0.0f,tiny,-tiny},
        {inf,0,0},{nan,1,2},{tiny,tiny,tiny},{0,-1,0},{0,0,-1}};
    const float totals[]={0,-0.0f,1,-1,tiny,inf,nan,1000};
    unsigned failures=0,cases=0;
    std::uint32_t random=0x72ae13;
    for (unsigned short pc:precisions) for (unsigned short rc=0;rc<0x1000;rc+=0x400)
        for (unsigned sample=0;sample<2048;++sample) {
        __asm { fldcw measurement }
        float words[38];
        for (float& value:words) { random^=random<<13;random^=random>>17;random^=random<<5;value=(static_cast<int>(random%65537)-32768)/1024.0f; }
        State seed;std::memcpy(&seed,words,sizeof(seed));
        if (sample<10) seed.vectors[0]=special[sample];
        if (sample<8) seed.scalars[19]=totals[sample];
        const unsigned variant=sample%6;
        const unsigned short control=static_cast<unsigned short>((original&~0xf00)|pc|rc|0x3f);
        for (unsigned op=0;op<5;++op) {
            State old,now;std::memcpy(&old,&seed,sizeof(seed));std::memcpy(&now,&seed,sizeof(seed));
            __asm { fldcw control }
            Run(true,op,old,variant);Run(false,op,now,variant);
            __asm { fldcw measurement }
            float a[38],b[38];std::memcpy(a,&old,sizeof(a));std::memcpy(b,&now,sizeof(b));
            bool good=true;
            // A published time can cancel a large chord/offset. Bound its
            // absolute error by the input magnitudes, not the tiny residual.
            double inputScale=0;
            if (op==1) {
                const float inputs[]={seed.vectors[0].X,seed.vectors[0].Y,seed.vectors[0].Z,
                    seed.vectors[1].X,seed.vectors[1].Y,seed.vectors[1].Z,seed.scalars[0],seed.scalars[1]};
                for (float value:inputs) if (std::isfinite(value)) inputScale+=std::fabs(static_cast<double>(value));
            }
            for (unsigned index=0;index<38;++index) {
                good &= Close(a[index],b[index],inputScale);
                const bool written=op==0 ? index>=29 : op==1 ? index<6 || (index>=9 && index<=11) :
                    op==2 ? index>=9 && index<26 : index/3==variant%3;
                if (!written) good &= std::memcmp(a+index,b+index,4)==0;
            }
            if (!good && failures++<10) {
                std::printf("sampling mismatch pc=%x rc=%x op=%u sample=%u alias=%u\n",pc,rc,op,sample,variant);
                for (unsigned index=0;index<38;++index)
                    if (!Close(a[index],b[index],inputScale)) std::printf("  lane=%u old=%.9g new=%.9g\n",index,a[index],b[index]);
            }
            ++cases;
        }
    }
    __asm { fnclex }
    __asm { fldcw original }
    std::printf("%u sampling cases; %u failures\n",cases,failures);
    return failures?1:0;
}
