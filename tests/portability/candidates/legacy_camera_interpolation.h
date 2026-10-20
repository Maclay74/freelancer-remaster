    __declspec(noinline) void LegacyCameraInterpolation(float* ResultValues, const float* TargetValues,
                                const float* CapturedValues,
                                const FThornExtendedProgress* Progress, int Mask)
    {
        // 46318..46374: preserve one extended curve result under each pair
        // of loads. Only the final interpolated field receives a float store.
        __asm
        {
            mov ecx, Progress
            fld tbyte ptr [ecx]
            mov ecx, CapturedValues
            mov edx, TargetValues
            mov edi, ResultValues
            mov eax, Mask
            test al, 1
            jz CameraInterpolationAspect
            fld dword ptr [ecx]
            fld dword ptr [edx]
            fsub st(0), st(1)
            fmul st(0), st(2)
            fadd st(0), st(1)
            fstp dword ptr [edi]
            fstp st(0)
        CameraInterpolationAspect:
            test al, 8
            jz CameraInterpolationNear
            fld dword ptr [ecx + 0ch]
            fld dword ptr [edx + 0ch]
            fsub st(0), st(1)
            fmul st(0), st(2)
            fadd st(0), st(1)
            fstp dword ptr [edi + 0ch]
            fstp st(0)
        CameraInterpolationNear:
            test al, 2
            jz CameraInterpolationFar
            fld dword ptr [ecx + 4]
            fld dword ptr [edx + 4]
            fsub st(0), st(1)
            fmul st(0), st(2)
            fadd st(0), st(1)
            fstp dword ptr [edi + 4]
            fstp st(0)
        CameraInterpolationFar:
            test al, 4
            jz CameraInterpolationDone
            fld dword ptr [ecx + 8]
            fld dword ptr [edx + 8]
            fsub st(0), st(1)
            fmul st(0), st(2)
            fadd st(0), st(1)
            fstp dword ptr [edi + 8]
            fstp st(0)
        CameraInterpolationDone:
            fstp st(0)
        }
    }
