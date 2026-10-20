    __declspec(noinline) void LegacySpatialWrapScale(float* Last, const FThornExtendedProgress* Progress, float* RoundedProgress, float* ResultScale)
    {
        static const float Zero = 0.0f;
        static const float One = 1.0f;
        static const double OneDouble = 1.0;
        float T;
        float Delta;
        float Scale;
        __asm
        {
            mov ecx, Last
            mov edx, Progress
            fld tbyte ptr [edx]
            fst T
            fsub dword ptr [ecx]
            fst Delta
            fcomp Zero
            fnstsw ax
            test ah, 44h
            // Equal: AH&44h=40h, odd parity -> JNP. Unordered=44h
            // has even parity and continues to the T comparison.
            jnp SpatialScaleZero
            fld T
            fcomp OneDouble
            fnstsw ax
            test ah, 05h
            // Greater/equal=00h and unordered=05h have even parity.
            // Less=01h has odd parity and enters the remainder branch.
            jp SpatialScaleOne
            fld One
            fsub dword ptr [ecx]
            fcom Zero
            fnstsw ax
            test ah, 44h
            // Equal=40h (odd) falls through; nonzero/unordered (even)
            // keeps the unrounded remainder on ST1 for FDIV.
            jp SpatialScaleDivide
            mov edx, T
            fstp st(0)
            mov Scale, 3f800000h
            mov dword ptr [ecx], edx
            jmp SpatialScaleDone
        SpatialScaleDivide:
            fld Delta
            mov edx, T
            fdiv st(0), st(1)
            mov dword ptr [ecx], edx
            fstp Scale
            fstp st(0)
            jmp SpatialScaleDone
        SpatialScaleOne:
            mov Scale, 3f800000h
            jmp SpatialScaleDone
        SpatialScaleZero:
            mov Scale, 0
        SpatialScaleDone:
        }
        // Retail retains exactly the initial FST dword for the later axis pass.
        std::memcpy(RoundedProgress, &T, sizeof(T));
        std::memcpy(ResultScale, &Scale, sizeof(Scale));
    }

    // LAB_06f498ad..49918 retains original extended T below Delta/Remain;
    // T is not stored as float until the conditional final Last update.
    __declspec(noinline) void LegacyFloorWrapScale(FThornEvent* Event, const FThornExtendedProgress* Progress, float* ScaleOutput)
    {
        float* Last = &Event->FloorWrapLast;
        const float Zero = 0.0f;
        const float One = 1.0f;
        const double OneDouble = 1.0;
        float Delta;
        float Scale;
        __asm
        {
            mov ecx, Last
            mov edx, Progress
            fld tbyte ptr [edx]
            fld st(0)
            fsub dword ptr [ecx]
            fst Delta
            fcomp Zero
            fnstsw ax
            test ah, 44h
            // Equality=40h, odd parity -> JNP; unordered=44h continues.
            jnp FloorScaleZero
            fcom OneDouble
            fnstsw ax
            test ah, 05h
            // Greater/equal=00h and unordered=05h -> even parity JP.
            jp FloorScaleOne
            fld One
            fsub dword ptr [ecx]
            fcom Zero
            fnstsw ax
            test ah, 44h
            // Nonzero/unordered has even parity -> divide; zero falls through.
            jp FloorScaleDivide
            fstp st(0)
            mov Scale, 3f800000h
            fstp dword ptr [ecx]
            jmp FloorScaleDone
        FloorScaleDivide:
            fld Delta
            fdiv st(0), st(1)
            fstp Scale
            fstp st(0)
            fstp dword ptr [ecx]
            jmp FloorScaleDone
        FloorScaleOne:
            mov Scale, 3f800000h
            jmp FloorScalePopT
        FloorScaleZero:
            mov Scale, 0
        FloorScalePopT:
            fstp st(0)
        FloorScaleDone:
        }
        // Transport the stored DWORD without an early x87 denormal load.
        std::memcpy(ScaleOutput, &Scale, sizeof(Scale));
    }
