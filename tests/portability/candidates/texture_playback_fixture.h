#pragma once
#include "../../../src/include/portable_integer_conversion.h"
#include "../../../src/Shading/TexturePlaybackMath.h"
#include <vector>

// Only the playback fields used by these two functions, without renderer/COM dependencies.
struct FTextureEntry { std::vector<unsigned char> Frames; float TileRate; };
struct FTexturePlayback
{
    FTextureEntry* Source;
    float LocalRate;
    bool bUseLocalRate;
    float PlaybackDirection;
    unsigned CurrentFrame, FrameCount;
    double ElapsedTime, TotalDuration;
    int WrapMode;
    float PublishedSeconds;
    unsigned SetterCalls;
};

// Match the unchanged production SetPlaybackElapsed path, including the empty
// source's fmod and the low 32 bits of the truncating integer conversion.
static void SetPlaybackElapsed(FTexturePlayback* entry, float seconds)
{
    entry->PublishedSeconds = seconds;
    ++entry->SetterCalls;
    entry->ElapsedTime = static_cast<double>(seconds);
    const double rate = entry->bUseLocalRate ? entry->LocalRate : entry->Source->TileRate;
    const double frame = std::floor(std::fmod(rate * entry->ElapsedTime,
                                            static_cast<double>(entry->Source->Frames.size())));
    entry->CurrentFrame = static_cast<unsigned>(PortableIntegerConversion::ToInt64Bits(&frame));
}

#include "legacy_texture_playback.h"

[[maybe_unused]] __declspec(noinline) static void PortableAdvance(FTexturePlayback* entry, float milliseconds)
{
    if (entry->FrameCount == 0)
        entry->FrameCount = static_cast<unsigned>(entry->Source->Frames.size());
    if (entry->FrameCount <= 1) return;
    SetPlaybackElapsed(entry, TexturePlaybackMath::Advance(entry->ElapsedTime, entry->TotalDuration,
        entry->PlaybackDirection, entry->WrapMode, milliseconds));
}
