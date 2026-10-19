#include "candidates/texture_playback_fixture.h"
#include <cstdio>
#include <limits>

static bool Close(double a, double b)
{
    if (std::isnan(a)) return std::isnan(b);
    if (std::isinf(a)) return a == b;
    return std::fabs(a - b) <= 2e-6 * (1.0 + std::fabs(a));
}

static bool SameState(const FTexturePlayback& a, const FTexturePlayback& b)
{
    return a.FrameCount == b.FrameCount && a.CurrentFrame == b.CurrentFrame &&
        (a.PlaybackDirection == b.PlaybackDirection ||
         (std::isnan(a.PlaybackDirection) && std::isnan(b.PlaybackDirection))) &&
        a.SetterCalls == b.SetterCalls && Close(a.ElapsedTime, b.ElapsedTime) &&
        Close(a.PublishedSeconds, b.PublishedSeconds);
}

int main()
{
    unsigned short original;
    __asm { fnstcw original }
    const unsigned short measurement = static_cast<unsigned short>((original & ~0xf00) | 0x200 | 0x3f);
    const unsigned short precisions[] = {0, 0x200, 0x300};
    const float milliseconds[] = {0, 1, 16, 33.333f, 1000, -16, -1000, 100000};
    const float directions[] = {-1, 1, 0, 2};
    const float rates[] = {1, 24, 30, -24};
    unsigned failures = 0, cases = 0, timeChanges = 0;
    for (unsigned short pc : precisions) for (unsigned short rc = 0; rc < 0x1000; rc += 0x400)
        for (unsigned sample = 0; sample < 8192; ++sample)
    {
        __asm { fldcw measurement }
        FTextureEntry source;
        const unsigned count = sample % 64 < 3 ? sample % 64 : 2 + sample % 63;
        source.Frames.resize(count);
        source.TileRate = rates[(sample / 8) % 4];
        const double duration = static_cast<double>(count) / 24.0 - 1e-6;
        double elapsed;
        switch ((sample / 32) % 8) {
        case 0: elapsed = 0; break;
        case 1: elapsed = duration; break;
        case 2: elapsed = std::nextafter(duration, 0.0); break;
        case 3: elapsed = std::nextafter(duration, std::numeric_limits<double>::infinity()); break;
        case 4: elapsed = -duration; break;
        case 5: elapsed = duration * 3.5; break;
        case 6: elapsed = -duration * 2.5; break;
        default: elapsed = duration * (sample % 1024) / 1024.0; break;
        }
        FTexturePlayback old = {&source, 24, (sample & 1) != 0,
            directions[(sample / 128) % 4], 17, sample % 3 ? count : 0,
            elapsed, duration, static_cast<int>((sample / 4) % 5) - 1, -999, 0};
        FTexturePlayback now = old;
        const float ms = milliseconds[sample % 8];
        const unsigned short control = static_cast<unsigned short>((original & ~0xf00) | pc | rc | 0x3f);
        __asm { fldcw control }
        LegacyTexturePlayback::AdvancePlayback(&old, ms);
        PortableAdvance(&now, ms);
        __asm { fldcw measurement }
        if (old.PublishedSeconds != now.PublishedSeconds) ++timeChanges;
        const bool good = SameState(old, now);
        if (!good && failures++ < 12)
            std::printf("advance mismatch pc=%x rc=%x sample=%u mode=%d ms=%g elapsed=%.17g duration=%.17g old=%g/%u/%g new=%g/%u/%g\n",
                pc,rc,sample,old.WrapMode,ms,elapsed,duration,old.PublishedSeconds,old.CurrentFrame,
                old.PlaybackDirection,now.PublishedSeconds,now.CurrentFrame,now.PlaybackDirection);
        ++cases;
    }
    // Follow complete animations rather than restarting the timer each sample.
    // Initialize durations through the respective implementations as production does.
    for (unsigned short pc : precisions) for (unsigned short rc = 0; rc < 0x1000; rc += 0x400)
        for (int mode = 0; mode < 3; ++mode) for (unsigned variant = 0; variant < 8; ++variant)
    {
        __asm { fldcw measurement }
        FTextureEntry source;
        source.Frames.resize(variant < 4 ? 8 : 64);
        source.TileRate = rates[variant % 4];
        const unsigned short control = static_cast<unsigned short>((original & ~0xf00) | pc | rc | 0x3f);
        __asm { fldcw control }
        FTexturePlayback old = {&source, 30, (variant & 1) != 0, variant & 2 ? -1.0f : 1.0f,
            0, 0, 0, 0, mode, 0, 0};
        FTexturePlayback now = old;
        const float rate = old.bUseLocalRate ? old.LocalRate : source.TileRate;
        old.TotalDuration = LegacyTexturePlayback::PlaybackDuration(static_cast<unsigned>(source.Frames.size()), rate);
        now.TotalDuration = TexturePlaybackMath::Duration(static_cast<unsigned>(source.Frames.size()), rate);
        for (unsigned step = 0; step < 512; ++step) {
            __asm { fldcw control }
            const float ms = step % 97 == 0 ? 1000.0f : 16.6667f;
            LegacyTexturePlayback::AdvancePlayback(&old, ms);
            PortableAdvance(&now, ms);
            __asm { fldcw measurement }
            if (old.PublishedSeconds != now.PublishedSeconds) ++timeChanges;
            if (!SameState(old, now) && failures++ < 12)
                std::printf("sequence mismatch pc=%x rc=%x mode=%d variant=%u step=%u old=%g/%u/%g new=%g/%u/%g\n",
                    pc,rc,mode,variant,step,old.PublishedSeconds,old.CurrentFrame,old.PlaybackDirection,
                    now.PublishedSeconds,now.CurrentFrame,now.PlaybackDirection);
            ++cases;
        }
    }
    // Malformed/exceptional state still follows the old ordered comparisons.
    const double nan = std::numeric_limits<double>::quiet_NaN();
    const double inf = std::numeric_limits<double>::infinity();
    for (int mode = -1; mode <= 3; ++mode) for (unsigned sample = 0; sample < 10; ++sample) {
        __asm { fldcw measurement }
        FTextureEntry source;
        source.Frames.resize(sample == 9 ? 0 : 8);
        source.TileRate = 24;
        FTexturePlayback old = {&source, 30, false, sample == 8 ? static_cast<float>(nan) : 1.0f,
            0, 8, sample < 2 ? nan : sample == 2 ? inf : 0.5,
            sample == 3 ? nan : sample == 4 ? inf : sample == 5 ? 0.0 : 1.0, mode, 0, 0};
        FTexturePlayback now = old;
        const float ms = sample == 6 ? static_cast<float>(nan) : sample == 7 ? static_cast<float>(inf) : 16;
        LegacyTexturePlayback::AdvancePlayback(&old, ms);
        PortableAdvance(&now, ms);
        if (!SameState(old, now) && failures++ < 12)
            std::printf("exceptional mismatch mode=%d sample=%u\n",mode,sample);
        ++cases;
    }
    __asm { fnclex }
    __asm { fldcw original }
    std::printf("%u advance cases; %u failures; %u float-time differences\n",cases,failures,timeChanges);
    return failures ? 1 : 0;
}
