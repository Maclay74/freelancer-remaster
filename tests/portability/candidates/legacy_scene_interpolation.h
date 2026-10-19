#pragma once
namespace LegacySceneInterpolation
{
    __declspec(noinline) void FogLerpFloat(const float* From, const float* To, const float* U, float* ResultValue)
    {
        __asm
        {
            mov eax, From
            fld dword ptr [eax]
            mov eax, To
            fld dword ptr [eax]
            fsub st(0), st(1)
            mov eax, U
            fmul dword ptr [eax]
            fadd st(0), st(1)
            mov eax, ResultValue
            fstp dword ptr [eax]
            fstp st(0)
        }
    }

    __declspec(noinline) void InterpolateLightDirection(float* Out, const float* From, const float* To, const float* U)
    {
        float Capture[3];
        float DifferenceZ;
        float DeltaX;
        float Result[3];
        __asm
        {
            mov eax, To
            fld dword ptr [eax]
            mov edx, From
            mov ecx, dword ptr [edx]
            mov dword ptr Capture, ecx
            fsub dword ptr Capture
            mov ecx, dword ptr [edx + 4]
            fld dword ptr [eax + 4]
            mov edx, dword ptr [edx + 8]
            mov dword ptr Capture[4], ecx
            fsub dword ptr Capture[4]
            fld dword ptr [eax + 8]
            mov dword ptr Capture[8], edx
            fsub dword ptr Capture[8]
            fstp dword ptr DifferenceZ
            fxch st(1)
            mov eax, U
            fmul dword ptr [eax]
            fstp dword ptr DeltaX
            fmul dword ptr [eax]
            fld dword ptr DifferenceZ
            fmul dword ptr [eax]
            fld dword ptr DeltaX
            fadd dword ptr Capture
            fstp dword ptr Result
            mov eax, dword ptr Result
            fxch st(1)
            fadd dword ptr Capture[4]
            mov ecx, Out
            mov dword ptr [ecx], eax
            fstp dword ptr Result[4]
            mov eax, dword ptr Result[4]
            fadd dword ptr Capture[8]
            mov dword ptr [ecx + 4], eax
            fstp dword ptr Result[8]
            mov eax, dword ptr Result[8]
            mov dword ptr [ecx + 8], eax
        }
    }

    __declspec(noinline) void LerpSpatialPosition(float* ResultPosition, const float* From, const float* To, const float* Scale)
    {
        __asm
        {
            mov eax, ResultPosition
            mov ecx, From
            mov edx, To
            mov esi, Scale
            fld dword ptr [edx]
            fsub dword ptr [ecx]
            fmul dword ptr [esi]
            fadd dword ptr [ecx]
            fstp dword ptr [eax]
            fld dword ptr [edx+4]
            fsub dword ptr [ecx+4]
            fmul dword ptr [esi]
            fadd dword ptr [ecx+4]
            fstp dword ptr [eax+4]
            fld dword ptr [edx+8]
            fsub dword ptr [ecx+8]
            fmul dword ptr [esi]
            fadd dword ptr [ecx+8]
            fstp dword ptr [eax+8]
        }
    }

    __declspec(noinline) void AccumulateAxisDelta(const float* AnimT, const float* Angle, float* Applied, float* Delta)
    {
        __asm
        {
            mov eax, Applied
            mov ecx, AnimT
            mov edx, Angle
            fld dword ptr [ecx]
            fmul dword ptr [edx]
            fsub dword ptr [eax]
            mov ecx, Delta
            fst dword ptr [ecx]
            fadd dword ptr [eax]
            fstp dword ptr [eax]
        }
    }

    __declspec(noinline) bool BuildAttachLookDirection(float* DirectionOut, const float* Target, const float* Origin)
    {
        unsigned char bUseDirection;
        const float Zero = 0.0f;
        __asm
        {
            mov ecx, Target
            mov edx, Origin
            mov eax, DirectionOut
            fld dword ptr [ecx+8]
            fsub dword ptr [edx+8]
            fld dword ptr [ecx+4]
            fsub dword ptr [edx+4]
            fld dword ptr [ecx]
            fsub dword ptr [edx]
            fstp dword ptr [eax]
            fst dword ptr [eax+4]
            fld st(1)
            fstp dword ptr [eax+8]
            fld dword ptr [eax]
            fmul dword ptr [eax]
            fld st(1)
            fmul st(0), st(2)
            faddp st(1), st(0)
            fld st(2)
            fmul st(0), st(3)
            faddp st(1), st(0)
            fcomp dword ptr Zero
            fnstsw ax
            fstp st(0)
            test ah, 44h
            fstp st(0)
            setp bUseDirection
        }
        return bUseDirection != 0;
    }

    __declspec(noinline) void InterpolateFloorHeight(const float* Capture, const float* Target, const float* ScaleSource, float* HeightOutput)
    {
        // 49b31..49b4a: Capture remains below the unrounded difference/product.
        float Height;
        __asm
        {
            mov ecx, Capture
            fld dword ptr [ecx]
            mov edx, Target
            fld dword ptr [edx]
            fsub st(0), st(1)
            mov ecx, ScaleSource
            fmul dword ptr [ecx]
            fadd st(0), st(1)
            fstp Height
            fstp st(0)
        }
        std::memcpy(HeightOutput, &Height, sizeof(Height));
    }

    __declspec(noinline) void ConvertCompactLightRetail(const unsigned char* Source, unsigned char* Target)
    {
        // Full98 retail instructions: selective writes and literal x87 branches.
        // TEST AH,05h + JP takes equal/greater/unordered; strict less falls through.
        const unsigned int Constant_6f5a1d4 = 0x0u;
        const unsigned int Constant_6f5a230 = 0x3c8efa35u;
        const unsigned long long Constant_6f5a270 = 0x400921fb60000000ull;
        const unsigned int Constant_6f5a26c = 0x3f8ccccdu;
        const unsigned int Constant_6f5a268 = 0x40490fdbu;
        const unsigned int Constant_6f5a1e8 = 0x3b808081u;
        const unsigned long long Constant_6f5a260 = 0x3f1a36e2eb1c432dull;
        const unsigned long long Constant_6f5a258 = 0x47efffffe0000000ull;
        const unsigned int Constant_6f5a250 = 0x3e800000u;
        const unsigned int Constant_6f5a1b8 = 0x3f800000u;
        float Scratch[4];
        __asm
        {
            mov edx, Source
            mov ecx, Target
            lea ebx, Scratch
            mov eax, dword ptr [edx + 024h]
            test eax, eax
            je Compact_6f2eb3a
            mov dword ptr [ecx], 3
            jmp Compact_6f2eba2
            Compact_6f2eb3a:
            fld dword ptr [edx + 01ch]
            fcomp dword ptr Constant_6f5a1d4
            fnstsw ax
            test ah, 5
            jp Compact_6f2eb5d
            mov eax, 040490fdbh
            mov dword ptr [ecx], 1
            mov dword ptr [ecx + 060h], eax
            mov dword ptr [ecx + 064h], eax
            jmp Compact_6f2eba2
            Compact_6f2eb5d:
            mov dword ptr [ecx], 2
            fld dword ptr [edx + 01ch]
            fmul dword ptr Constant_6f5a230
            fcom qword ptr Constant_6f5a270
            fnstsw ax
            test ah, 5
            jnp Compact_6f2eb81
            fstp st(0)
            fld qword ptr Constant_6f5a270
            Compact_6f2eb81:
            fst dword ptr [ecx + 060h]
            fmul dword ptr Constant_6f5a26c
            fcom dword ptr Constant_6f5a268
            fnstsw ax
            test ah, 5
            jnp Compact_6f2eb9f
            fstp st(0)
            fld dword ptr Constant_6f5a268
            Compact_6f2eb9f:
            fstp dword ptr [ecx + 064h]
            Compact_6f2eba2:
            fild dword ptr [edx]
            lea eax, [ecx + 4]
            mov dword ptr [ebx + 0ch], 03f800000h
            fmul dword ptr Constant_6f5a1e8
            fstp dword ptr [ebx]
            mov esi, dword ptr [ebx]
            fild dword ptr [edx + 4]
            fmul dword ptr Constant_6f5a1e8
            fstp dword ptr [ebx + 4]
            fild dword ptr [edx + 8]
            mov dword ptr [eax], esi
            mov esi, dword ptr [ebx + 4]
            mov dword ptr [eax + 4], esi
            fmul dword ptr Constant_6f5a1e8
            fstp dword ptr [ebx + 8]
            mov esi, dword ptr [ebx + 8]
            mov dword ptr [eax + 8], esi
            mov esi, dword ptr [ebx + 0ch]
            mov dword ptr [eax + 0ch], esi
            xor eax, eax
            lea esi, [ecx + 014h]
            mov dword ptr [esi], eax
            mov dword ptr [esi + 4], eax
            mov dword ptr [esi + 8], eax
            mov dword ptr [esi + 0ch], eax
            lea esi, [ecx + 024h]
            mov dword ptr [esi], eax
            mov dword ptr [esi + 4], eax
            mov dword ptr [esi + 8], eax
            mov dword ptr [esi + 0ch], eax
            mov esi, 03f800000h
            mov dword ptr [ecx + 050h], esi
            fld dword ptr [edx + 018h]
            fcomp qword ptr Constant_6f5a260
            fnstsw ax
            test ah, 5
            jp Compact_6f2ec4a
            fld qword ptr Constant_6f5a258
            mov dword ptr [ecx + 058h], 038d1b717h
            fsqrt
            mov dword ptr [ecx + 054h], 0
            mov dword ptr [ecx + 05ch], 0
            fstp dword ptr [ecx + 04ch]
            jmp CompactDone
            Compact_6f2ec4a:
            mov dword ptr [ecx + 054h], esi
            mov dword ptr [ecx + 058h], 0
            fld dword ptr [edx + 018h]
            fld st(0)
            fmul st(0), st(1)
            fmul dword ptr Constant_6f5a250
            fdivr dword ptr Constant_6f5a1b8
            fstp dword ptr [ecx + 05ch]
            fstp st(0)
            fld dword ptr [edx + 018h]
            fadd st(0), st(0)
            fstp dword ptr [ecx + 04ch]
            jmp CompactDone
        CompactDone:
        }
    }

    __declspec(noinline) void ApplyPsysSparam(FThornEntity* Entity, const float* Value)
    {
        float Argument;
        float* Runtime = &Entity->PsysRuntimeSparam;
        __asm {
            mov eax, Value
            fld dword ptr [eax]
            fst dword ptr Argument
            mov edx, Runtime
            fstp dword ptr [edx]
        }
        // The pointer is read only after the runtime float has been published.
        IPSysControl* Control = Entity->PsysControl;
        if (Control != nullptr)
        {
            // Typed public slot6; the argument was already rounded above.
            Control->SetSparam(Argument);
        }
    }

__declspec(noinline) void BuildAxisRotation(float* output,int AxisId,const float* DeltaDeg)
{
        const unsigned int PiOver180Bits = 0x3c8efa35u;
        const unsigned int Kind = static_cast<unsigned int>(AxisId);
        float Rot[9];
        InitializeEventFrameWords(Rot);
        if (Kind <= 5u)
        {
            float CosValue;
            __asm
            {
                mov eax, DeltaDeg
                fld dword ptr [eax]
                fmul dword ptr PiOver180Bits
                fld st(0)
                fcos
                fstp dword ptr CosValue
                fsin
                mov edx, Kind
                cmp edx, 0
                je AxisXColumns
                cmp edx, 3
                je AxisXColumns
                cmp edx, 1
                je AxisYColumns
                cmp edx, 4
                je AxisYColumns
                lea eax, Rot
                add eax, 12
                mov ecx, 3
            AxisZLoop:
                fld dword ptr [eax-12]
                add eax, 4
                dec ecx
                fld dword ptr CosValue
                fmul st(0), st(1)
                fld st(2)
                fmul dword ptr [eax-4]
                fsubp st(1), st(0)
                fstp dword ptr [eax-16]
                fld dword ptr CosValue
                fmul dword ptr [eax-4]
                fxch st(1)
                fmul st(0), st(2)
                faddp st(1), st(0)
                fstp dword ptr [eax-4]
                jnz AxisZLoop
                jmp AxisColumnsDone
            AxisXColumns:
                lea eax, Rot
                add eax, 24
                mov ecx, 3
            AxisXLoop:
                fld dword ptr [eax-12]
                add eax, 4
                dec ecx
                fld dword ptr CosValue
                fmul st(0), st(1)
                fld st(2)
                fmul dword ptr [eax-4]
                fsubp st(1), st(0)
                fstp dword ptr [eax-16]
                fmul st(0), st(1)
                fld dword ptr CosValue
                fmul dword ptr [eax-4]
                faddp st(1), st(0)
                fstp dword ptr [eax-4]
                jnz AxisXLoop
                jmp AxisColumnsDone
            AxisYColumns:
                lea eax, Rot
                add eax, 24
                mov ecx, 3
            AxisYLoop:
                fld dword ptr [eax-24]
                add eax, 4
                dec ecx
                fld st(1)
                fmul dword ptr [eax-4]
                fld st(1)
                fmul dword ptr CosValue
                faddp st(1), st(0)
                fstp dword ptr [eax-28]
                fld dword ptr CosValue
                fmul dword ptr [eax-4]
                fxch st(1)
                fmul st(0), st(2)
                fsubp st(1), st(0)
                fstp dword ptr [eax-4]
                jnz AxisYLoop
            AxisColumnsDone:
                fstp st(0)
            }
        }

    std::memcpy(output,Rot,sizeof(Rot));
}

__declspec(noinline) void ParticleLerp(const float* Capture,float Target,float Scale,float* output)
{
    float Result;
                    __asm {
                        mov edx, Capture
                        fld dword ptr [edx]
                        fld dword ptr Target
                        fsub st(0), st(1)
                        fmul dword ptr Scale
                        fadd st(0), st(1)
                        fstp dword ptr Result
                        fstp st(0)
                    }
    std::memcpy(output,&Result,sizeof(Result));
}

__declspec(noinline) void NegatePosition(float* PositionResult,const float* PositionSource)
{
    __asm {
        mov ecx, PositionSource
        mov eax, PositionResult
        fld dword ptr [ecx+8]
        fchs
        fstp dword ptr [eax+8]
        fld dword ptr [ecx+4]
        fchs
        fstp dword ptr [eax+4]
        fld dword ptr [ecx]
        fchs
        fstp dword ptr [eax]
    }
}
}
