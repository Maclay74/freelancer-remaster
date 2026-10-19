// The repository's release headers retain iterator debugging level 1. Let
// actual /MDd builds use their normal level 2; the isolated /MD runner uses 1.
#if !defined(_DEBUG) && !defined(_ITERATOR_DEBUG_LEVEL)
#define _ITERATOR_DEBUG_LEVEL 1
#endif
#include "../../src/include/rpc_packet_codec.h"
#include <windows.h>
#include <xmmintrin.h>
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <initializer_list>

// Minimal packet lifecycle for the isolated real inline wire method. No
// transport code is linked; unexpected virtual dispatch aborts the test.
FRpcRefObject::~FRpcRefObject() = default;
FPacketBase::FPacketBase(unsigned int id) : Field_0x08(nullptr), PacketId(id),
    MarshalCursor(0), MarshalLimit(0), MarshalPtr(0), MarshalError(0) {}
FPacketBase::~FPacketBase() = default;
unsigned int FPacketBase::Encode(unsigned char**, unsigned int, unsigned int*) { std::abort(); }
unsigned int FPacketBase::Decode(const unsigned char*, unsigned int, unsigned int*) { std::abort(); }
void FPacketBase::Dump(std::ostream&) const { std::abort(); }

__declspec(noinline) bool Reference(FPacketBase& Packet, const float& Value)
{
    Packet.MarshalCursor += 4u;
    if (Packet.MarshalCursor > Packet.MarshalLimit) Packet.MarshalError = 0x16u;
    if (Packet.MarshalError) return false;
    const float* Source = &Value;
    const unsigned int Cached = Packet.MarshalPtr;
    float* Destination = reinterpret_cast<float*>(Cached);
    __asm {
        mov eax, Source
        mov edx, Destination
        fld dword ptr [eax]
        fstp dword ptr [edx]
    }
    Packet.MarshalPtr = Cached + 4u;
    return true;
}

__declspec(noinline) void ClearFlags()
{
    __asm {
        fldz
        fstp st(0)
        fnclex
    }
}

using Kernel = bool (*)(FPacketBase&, FRpcWireStream&, const float&);
static bool ReferenceCall(FPacketBase& packet, FRpcWireStream&, const float& value) { return Reference(packet, value); }
static bool CandidateCall(FPacketBase&, FRpcWireStream& stream, const float& value) { return stream.PutFloat(value); }
static int CaptureException(EXCEPTION_POINTERS* information, unsigned short* status)
{
    *status = static_cast<unsigned short>(information->ContextRecord->FloatSave.StatusWord);
    __asm { fnclex }
    return EXCEPTION_EXECUTE_HANDLER;
}
static unsigned RunUnmasked(Kernel kernel, FPacketBase& packet, FRpcWireStream& stream, const float& value,
                            unsigned short control, unsigned short* status, bool* result)
{
    unsigned code = 0;
    ClearFlags();
    __asm { fldcw control }
    __try {
        *result = kernel(packet, stream, value);
        unsigned short completed;
        __asm { fnstsw completed }
        *status = completed;
    }
    __except (CaptureException(GetExceptionInformation(), status)) { code = GetExceptionCode(); }
    __asm { fninit }
    return code;
}

int main()
{
    unsigned short original;
    __asm { fnstcw original }
    const unsigned short precisions[] = {0, 0x200, 0x300};
    const unsigned cursors[] = {0, 1, 4, 0xfffffffcu, 0xfffffffeu};
    const unsigned limits[] = {0, 3, 4, 5, 0xffffffffu};
    const std::uint32_t patterns[] = {0, 0x80000000u, 0x3f800000u, 0xbf800000u,
        0x7f800000u, 0xff800000u, 0x7fc12345u, 0xffc54321u, 0x7f812345u, 0xff854321u,
        1, 0x80000001u, 0x007fffffu, 0x00800000u, 0x7f7fffffu};
    unsigned cases = 0;
    std::uint32_t random = 0xc192743bu;
    for (unsigned short pc : precisions) for (unsigned short rc = 0; rc < 0x1000; rc += 0x400)
    {
        const unsigned short control = static_cast<unsigned short>((original & ~0xf00) | pc | rc | 0x3f);
        __asm { fldcw control }
        for (unsigned sample = 0; sample < 100; ++sample) for (unsigned cursor : cursors) for (unsigned limit : limits)
            for (unsigned error : {0u, 0x16u, 0x17u}) for (unsigned offset = 0; offset < 4; ++offset) for (unsigned alias : {0u, 1u})
            {
                float a[8], b[8];
                std::memset(a, 0x5a, sizeof(a));
                random ^= random << 13; random ^= random >> 17; random ^= random << 5;
                const auto bits = sample < 15 ? patterns[sample] : random;
                float value;
                std::memcpy(&value, &bits, sizeof(bits));
                std::memcpy(a + 2, &bits, sizeof(bits));
                std::memcpy(b, a, sizeof(a));
                FPacketBase expected(1), actual(1);
                auto* aStart = reinterpret_cast<unsigned char*>(a + 2) + offset;
                auto* bStart = reinterpret_cast<unsigned char*>(b + 2) + offset;
                FRpcWireStream expectedStream(expected, static_cast<void*>(aStart), limit);
                FRpcWireStream actualStream(actual, static_cast<void*>(bStart), limit);
                expected.MarshalCursor = actual.MarshalCursor = cursor;
                expected.MarshalError = actual.MarshalError = error;
                unsigned short expectedStatus, actualStatus;
                ClearFlags();
                const unsigned mxcsr = _mm_getcsr();
                const bool resultA = Reference(expected, alias ? a[2] : value);
                __asm { fnstsw expectedStatus }
                ClearFlags();
                const bool resultB = actualStream.PutFloat(alias ? b[2] : value);
                __asm { fnstsw actualStatus }
                if (resultA != resultB || expected.MarshalCursor != actual.MarshalCursor ||
                    expected.MarshalError != actual.MarshalError ||
                    expected.MarshalPtr - reinterpret_cast<unsigned>(aStart) != actual.MarshalPtr - reinterpret_cast<unsigned>(bStart) ||
                    std::memcmp(a, b, sizeof(a)) || expectedStatus != actualStatus || mxcsr != _mm_getcsr())
                {
                    std::fprintf(stderr, "Mismatch sample=%u cursor=%u limit=%u error=%u offset=%u alias=%u status=%x/%x\n",
                                 sample, cursor, limit, error, offset, alias, expectedStatus, actualStatus);
                    return 1;
                }
                ++cases;
            }
    }
    unsigned exceptional = 0;
    for (unsigned short pc : precisions) for (unsigned short rc = 0; rc < 0x1000; rc += 0x400)
        for (unsigned mask : {1u, 2u}) for (unsigned cursor : cursors) for (unsigned limit : limits)
            for (unsigned error : {0u, 0x16u, 0x17u}) for (unsigned offset = 0; offset < 4; ++offset) for (unsigned alias : {0u, 1u})
            {
                float a[8], b[8];
                std::memset(a, 0x5a, sizeof(a));
                const std::uint32_t bits = mask == 1 ? 0x7f812345u : 1u;
                float value;
                std::memcpy(&value, &bits, sizeof(bits));
                std::memcpy(a + 2, &bits, sizeof(bits));
                std::memcpy(b, a, sizeof(a));
                FPacketBase expected(1), actual(1);
                auto* aStart = reinterpret_cast<unsigned char*>(a + 2) + offset;
                auto* bStart = reinterpret_cast<unsigned char*>(b + 2) + offset;
                FRpcWireStream expectedStream(expected, static_cast<void*>(aStart), limit);
                FRpcWireStream actualStream(actual, static_cast<void*>(bStart), limit);
                expected.MarshalCursor = actual.MarshalCursor = cursor;
                expected.MarshalError = actual.MarshalError = error;
                const unsigned short control = static_cast<unsigned short>(((original & ~0xf00) | pc | rc | 0x3f) & ~mask);
                unsigned short expectedStatus, actualStatus;
                bool resultA = false, resultB = false;
                const auto codeA = RunUnmasked(ReferenceCall, expected, expectedStream, alias ? a[2] : value, control, &expectedStatus, &resultA);
                const auto codeB = RunUnmasked(CandidateCall, actual, actualStream, alias ? b[2] : value, control, &actualStatus, &resultB);
                if (codeA != codeB || (!codeA && resultA != resultB) || expectedStatus != actualStatus ||
                    expected.MarshalCursor != actual.MarshalCursor || expected.MarshalError != actual.MarshalError ||
                    expected.MarshalPtr - reinterpret_cast<unsigned>(aStart) != actual.MarshalPtr - reinterpret_cast<unsigned>(bStart) ||
                    std::memcmp(a, b, sizeof(a)))
                {
                    std::fprintf(stderr, "Unmasked mismatch mask=%u cursor=%u limit=%u error=%u offset=%u alias=%u codes=%x/%x status=%x/%x\n",
                                 mask, cursor, limit, error, offset, alias, codeA, codeB, expectedStatus, actualStatus);
                    return 2;
                }
                ++exceptional;
            }
    ClearFlags();
    __asm { fldcw original }
    std::printf("PASS: %u exact wire-return/cursor/error/pointer/buffer/status comparisons\n", cases);
    std::printf("PASS: %u unmasked cases including preflight failures and pointer commit order\n", exceptional);
    return 0;
}
