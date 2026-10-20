// Compile the production scene kernels; discard unrelated engine entry points.
#include "../../src/Thorn/ThornScene.cpp"
#include <cstdio>

// None of these engine boundaries should be reached by arithmetic tests.
I3DMathEngine* ThornAcquire3DMathEngine() { std::abort(); }
void ThornSetReverbProperties(IReverbControl*, const FThornReverb4*) { std::abort(); }
void ThornFdump(const char*) { std::abort(); }
void ThornSetFloorHeight(ICompoundControl*, const float*) { std::abort(); }
void FThornModernAdapters::ReleaseOwnedComponent(IDAComponent*&) { std::abort(); }
bool ThornNameEqual(const char* a, const char* b) { return a && b && _stricmp(a, b) == 0; }

int main()
{
    int failures = 0;
    unsigned short before, after;
    __asm { fnstsw before }
    for (float progress : {0.0f, 0.25f, 0.5f, 0.75f, 1.0f})
    {
        const double actual = ComputeLightColor(20, 220, &progress);
        const double expected = 20 + 200 * progress;
        if (actual != expected)
        {
            std::fprintf(stderr, "Light at %g: got %g, expected %g\n", progress, actual, expected);
            ++failures;
        }
        const int audio = ComputeAudioLerpInteger(20, 220, progress);
        if (audio != expected)
        {
            std::fprintf(stderr, "Audio at %g: got %g, expected %g\n", progress, static_cast<double>(audio), expected);
            ++failures;
        }
        if (ComputeLightColor(220, 20, &progress) != 220 - 200 * progress)
            ++failures;
        FThornLightProps from{}, to{}, output{};
        for (int channel = 0; channel < 3; ++channel)
        {
            from.Color[channel] = 20;
            to.Color[channel] = 220;
        }
        const volatile int mask = kThornLightBitColor;
        EvalLightProps(&output, &from, &to, &mask, &progress);
        for (int color : output.Color)
            if (color != expected) ++failures;
    }
    FThornEvent event(kThornTypeFloorHeightAnim);
    event.Desc.StartMs = 100;
    event.Desc.DurationMs = 400;
    event.StartPercent = 0.125f;
    for (int time : {100, 200, 300, 400, 500})
    {
        const float expected = (time - 100) / 400.0f;
        const float progress = event.EvaluateProgress(time);
        const float noWrap = event.EvaluateProgressNoPeriodWrap(time);
        const float scaled = event.EvaluateControlBackedCurve(0.5f, time);
        if (progress != expected || noWrap != expected || scaled != 0.125f + 0.5f * expected)
        {
            std::fprintf(stderr, "Event at %d: progress %g, noWrap %g, scaled %g (expected %g)\n",
                         time, progress, noWrap, scaled, expected);
            ++failures;
        }
    }
    event.Desc.DurationMs = 0;
    if (event.EvaluateProgress(100) != 1 || event.EvaluateProgressNoPeriodWrap(100) != 1 ||
        event.EvaluateControlBackedCurve(0, 100) != 1)
        ++failures;
    event.Desc.DurationMs = 400;
    event.ParamCurve = static_cast<IParamCurve*>(ThornCreateParamCurve("SmoothPCurve", false));
    for (int period : {0, 200})
    {
        event.PcurvePeriod = period;
        for (int time : {100, 150, 200, 250, 300, 350, 400})
        {
            const float t = period ? ((time - 100) % period) / float(period) : (time - 100) / 400.0f;
            const float expected = t * t * (3 - 2 * t);
            if (event.EvaluateProgress(time) != expected ||
                event.EvaluateControlBackedCurve(0.5f, time) != 0.125f + 0.5f * expected)
                ++failures;
            const float unwrapped = (time - 100) / 400.0f;
            if (event.EvaluateProgressNoPeriodWrap(time) != unwrapped * unwrapped * (3 - 2 * unwrapped))
                ++failures;
        }
    }
    __asm { fnstsw after }
    if ((before & 0x3800) != (after & 0x3800))
    {
        std::fprintf(stderr, "Unbalanced x87 stack: %04x -> %04x\n", before, after);
        ++failures;
    }
    if (failures) std::fprintf(stderr, "%d scene arithmetic failures\n", failures);
    return failures ? 1 : 0;
}
