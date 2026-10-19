#include "../../src/Thorn/ParamCurve.cpp"
#include <cstdio>

bool ThornNameEqual(const char* a, const char* b) { return a && b && _stricmp(a, b) == 0; }
static float ReadExtended(const FThornExtendedProgress& value)
{
    const void* input = &value;
    float result;
    __asm {
        mov eax, input
        fld tbyte ptr [eax]
        fstp result
    }
    return result;
}
int main()
{
    const char* factories[] = {"SmoothPCurve", "StepPCurve", "RampUpPCurve", "RampDownPCurve",
        "BumpOutPCurve", "BumpInPCurve", "FreeFormPCurve", "LinearPCurve", "CatmullRomPCurve"};
    const float times[] = {0, 0.25f, 0.5f, 0.75f, 1};
    for (const char* factory : factories) {
        FParamCurve curve(SpecFor(factory, false));
        for (float time : times) {
            FThornExtendedProgress extended;
            curve.EvaluateToExtended(&time, &extended);
            const float value = ReadExtended(extended);
            // The retail linear evaluator finds the first strictly later key;
            // at the final key it deliberately returns zero.
            const float end = SpecFor(factory, false)->Eval == EvalLerp ? 0.0f : 1.0f;
            if (!std::isfinite(value) || (time == 0 && value != 0) || (time == 1 && value != end)) {
                std::fprintf(stderr, "%s at %g produced %g\n", factory, time, value);
                return 1;
            }
            const float scalar = curve.Evaluate(time);
            if (scalar != value) { std::fprintf(stderr, "%s at %g scalar %g != extended %g\n", factory, time, scalar, value); return 2; }
        }
    }
    return 0;
}
