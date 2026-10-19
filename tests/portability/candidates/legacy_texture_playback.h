#pragma once

// Test-only snapshots of the removed texture playback assembly.
namespace LegacyTexturePlayback
{
    __declspec(noinline) double PlaybackDuration(unsigned int Count, float Rate)
    {
        const unsigned long long IntegerCount = Count;
        const double Epsilon = 1e-6;
        double Duration;
        __asm
        {
            fild IntegerCount
            fdiv Rate
            fsub Epsilon
            fstp Duration
        }
        return Duration;
    }

    __declspec(noinline) void AdvancePlayback(FTexturePlayback* Entry, float Milliseconds)
    {
        // FUN_06ebe660 retains x87 precision across FST double and later comparisons.
        if (Entry->FrameCount == 0)
            Entry->FrameCount = (unsigned int)Entry->Source->Frames.size();
        if (Entry->FrameCount <= 1)
            return;
        const float SecondsPerMillisecond = 0.001f;
        const double Zero = 0.0;
        double Delta;
        __asm
        {
            fld Milliseconds
            fmul SecondsPerMillisecond
            fstp Delta
        }
        double* Elapsed = &Entry->ElapsedTime;
        const double* Duration = &Entry->TotalDuration;
        float* Direction = &Entry->PlaybackDirection;
        float Seconds = 0.0f;
        switch (Entry->WrapMode)
        {
        case 0:
            __asm
            {
                mov ecx, Elapsed
                mov edx, Duration
                fld Delta
                fadd qword ptr [ecx]
                fst qword ptr [ecx]
                fld qword ptr [edx]
                fld st(1)
                fcompp
                fnstsw ax
                test ah, 5
                jnp ClampLower
                fstp st(0)
                fld qword ptr [edx]
            ClampLower:
                fcom Zero
                fnstsw ax
                test ah, 0x41
                jz ClampStore
                fstp st(0)
                fld Zero
            ClampStore:
                fstp Seconds
            }
            break;
        case 1:
        {
            double RoundedNext;
            __asm
            {
                mov ecx, Elapsed
                fld Delta
                fadd qword ptr [ecx]
                fst RoundedNext
                fstp qword ptr [ecx]
            }
            Seconds = (float)std::fmod(RoundedNext, *Duration);
            break;
        }
        case 2:
        {
            double RoundedNext;
            int Reflection = 0;
            __asm
            {
                mov ecx, Elapsed
                mov edx, Direction
                fld dword ptr [edx]
                fmul Delta
                fadd qword ptr [ecx]
                fst RoundedNext
                fst qword ptr [ecx]
                fld dword ptr [edx]
                fcomp Zero
                fnstsw ax
                test ah, 5
                jp ForwardTest
                fcom Zero
                fnstsw ax
                test ah, 5
                jp ForwardTest
                mov Reflection, 1
                fstp st(0)
                jmp ReflectionDone
            ForwardTest:
                fld dword ptr [edx]
                fcomp Zero
                fnstsw ax
                test ah, 0x41
                jnz KeepExtended
                mov ecx, Duration
                fcom qword ptr [ecx]
                fnstsw ax
                test ah, 0x41
                jnz KeepExtended
                mov Reflection, 2
                fstp st(0)
                jmp ReflectionDone
            KeepExtended:
                fstp Seconds
            ReflectionDone:
            }
            if (Reflection != 0)
            {
                const double Remainder = std::fmod(RoundedNext, *Duration);
                if (Reflection == 1)
                {
                    __asm
                    {
                        fld Remainder
                        fchs
                        fstp Seconds
                    }
                }
                else
                {
                    __asm
                    {
                        mov ecx, Duration
                        fld Remainder
                        fsubr qword ptr [ecx]
                        fstp Seconds
                    }
                }
                __asm
                {
                    mov ecx, Direction
                    fld dword ptr [ecx]
                    fchs
                    fstp dword ptr [ecx]
                }
            }
            break;
        }
        default:
            break;
        }
        SetPlaybackElapsed(Entry, Seconds);
    }
}
