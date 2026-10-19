#include "candidates/texture_playback_fixture.h"
#include <cstdio>
#include <limits>

__declspec(noinline) static double PortableDuration(unsigned count, float rate)
{
    return TexturePlaybackMath::Duration(count, rate);
}

int main()
{
    unsigned short original;
    __asm { fnstcw original }
    const unsigned short measurement = static_cast<unsigned short>((original & ~0xf00) | 0x200 | 0x3f);
    const unsigned short precisions[] = {0, 0x200, 0x300};
    const unsigned counts[] = {0, 1, 2, 24, 60, 16777215, 16777216, 16777217, 0x7fffffff, 0xffffffff};
    const float rates[] = {0, -0.0f, 0.1f, 1, -1, 24, 30, 60, 1e-5f, -1e-5f,
        std::numeric_limits<float>::denorm_min(), std::numeric_limits<float>::max(),
        std::numeric_limits<float>::infinity(), std::numeric_limits<float>::quiet_NaN()};
    std::uint32_t random = 0x52ab17;
    unsigned failures = 0, cases = 0;
    double maximum = 0;
    for (unsigned short pc : precisions) for (unsigned short rc = 0; rc < 0x1000; rc += 0x400)
        for (unsigned sample = 0; sample < 2048; ++sample)
    {
        __asm { fldcw measurement }
        random ^= random << 13; random ^= random >> 17; random ^= random << 5;
        const unsigned count = sample < 140 ? counts[sample / 14] : random;
        const float rate = sample < 140 ? rates[sample % 14] :
            static_cast<float>(static_cast<int>(random % 200001) - 100000) / 128.0f;
        const unsigned short control = static_cast<unsigned short>((original & ~0xf00) | pc | rc | 0x3f);
        __asm { fldcw control }
        const double old = LegacyTexturePlayback::PlaybackDuration(count, rate);
        const double now = PortableDuration(count, rate);
        __asm { fldcw measurement }
        bool good;
        if (std::isnan(old)) good = std::isnan(now);
        else if (std::isinf(old)) good = old == now;
        else {
            const double error = std::fabs(now - old) / (1.0 + std::fabs(old));
            if (error > maximum) maximum = error;
            good = error <= 2e-7;
        }
        if (!good && failures++ < 5)
            std::printf("duration mismatch pc=%x rc=%x count=%u rate=%g old=%.17g new=%.17g\n",pc,rc,count,rate,old,now);
        ++cases;
    }
    __asm { fnclex }
    __asm { fldcw original }
    std::printf("%u duration cases; %u failures; max scaled error %.9g\n",cases,failures,maximum);
    return failures ? 1 : 0;
}
