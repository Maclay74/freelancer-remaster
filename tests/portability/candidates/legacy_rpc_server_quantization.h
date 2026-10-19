// Original shared RPC quantizers; test-only assembly oracle.
#pragma once
namespace LegacyServerQuantization
{
static const double GRpcServerScale255 = 255.0;
static const double GRpcServerInverse255 = 1.0 / 255.0;
static const float GRpcServerSendCommTenth = 0.1f;
static const double GRpcServerSendCommScale255 = 255.0;
__declspec(noinline) unsigned char RpcServerQuantizeHull(int HitPts, int Maximum, unsigned short* CallbackStatus) {
    float Points;
    __asm {
        fild dword ptr HitPts
        fstp dword ptr Points
    }
    unsigned short BeforeLookup;
    __asm { fnstsw BeforeLookup }
    *CallbackStatus = BeforeLookup;
    const int MaxHitPts = Maximum;
    unsigned short OldControl;
    unsigned short TruncateControl;
    __int64 Quantized;
    __asm {
        fild dword ptr MaxHitPts
        fdivr dword ptr Points
        fmul qword ptr GRpcServerScale255
        fnstcw OldControl
        mov ax, OldControl
        or ax, 0c00h
        mov TruncateControl, ax
        fldcw TruncateControl
        fistp qword ptr Quantized
        fldcw OldControl
    }
    return static_cast<unsigned char>(Quantized);
}

__declspec(noinline) int RpcServerExpandHull(unsigned char Encoded, int Maximum) {
    const int MaxHitPts = Maximum;
    const int Widened = static_cast<int>(Encoded);
    unsigned short OldControl;
    unsigned short TruncateControl;
    __int64 Result;
    __asm {
        fild dword ptr MaxHitPts
        fild dword ptr Widened
        fmul qword ptr GRpcServerInverse255
        fmulp st(1), st
        fnstcw OldControl
        mov ax, OldControl
        or ax, 0c00h
        mov TruncateControl, ax
        fldcw TruncateControl
        fistp qword ptr Result
        fldcw OldControl
    }
    return static_cast<int>(Result);
}

__declspec(noinline) unsigned char RpcServerSendCommQuantize(const float* Value) {
    unsigned short OldControl;
    unsigned short TruncateControl;
    __int64 Quantized;
    __asm {
        mov ecx, Value
        fld dword ptr [ecx]
        fmul dword ptr GRpcServerSendCommTenth
        fmul qword ptr GRpcServerSendCommScale255
        fnstcw OldControl
        mov ax, OldControl
        or ax, 0c00h
        mov TruncateControl, ax
        fldcw TruncateControl
        fistp qword ptr Quantized
        fldcw OldControl
    }
    return static_cast<unsigned char>(Quantized);
}
}
