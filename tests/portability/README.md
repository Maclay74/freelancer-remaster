# Isolated production-kernel comparisons

No game build, DATA, DirectX SDK, or MFC is needed:

```powershell
.\scripts\test-portable-kernel.ps1 -Test hermite_assignment
.\scripts\test-portable-kernel.ps1 -Test scene_negate_vector
.\scripts\test-portable-kernel.ps1 -Test server_vector_copy
.\scripts\test-portable-kernel.ps1 -Test scene_read_column
.\scripts\test-portable-kernel.ps1 -Test scene_transpose
.\scripts\test-portable-kernel.ps1 -Test param_strict_less
.\scripts\test-portable-kernel.ps1 -Test scene_interpolation_complete
.\scripts\test-portable-kernel.ps1 -Test scene_nonzero_float
.\scripts\test-portable-kernel.ps1 -Test hermite_construction
.\scripts\test-portable-kernel.ps1 -Test lua_double_probe
.\scripts\test-portable-kernel.ps1 -Test lua_float_probe
.\scripts\test-portable-kernel.ps1 -Test spline_negate_stored
.\scripts\test-portable-kernel.ps1 -Test spline_double_stored
.\scripts\test-portable-kernel.ps1 -Test float_extended_storage
.\scripts\test-portable-kernel.ps1 -Test float_load_copy
.\scripts\test-portable-kernel.ps1 -Test wire_put_float
.\scripts\test-portable-kernel.ps1 -Test rpc_server_float_copy
```

Each command compiles only that standalone test with MSVC and clang-cl, x86
Release. Debug is available for investigating failures with
`-Configuration debug`, or both with `-Configuration release,debug`.
The test includes the actual production C++ kernel and a
retained assembly oracle. Results go to `out/tests/portability/<test>/results.txt`.
The verified tests also run through CTest. GitHub Actions is currently disabled;
the saved workflow is in `.github/disabled-workflows/windows-clang.yml`.

| Replacement | Comparisons per compiler/configuration |
| --- | --- |
| Hermite record assignment | 1,080,000 exact whole-buffer comparisons, special bit patterns and overlaps; unchanged x87 status/MXCSR |
| Scene vector negation | 43,008 exact whole-buffer/status comparisons per Release compiler, overlapping vectors and all precision/rounding modes; unmasked exception delivery untested |
| Server position copy | Same 43,008-case matrix with the original numerical load/store order; unmasked exception delivery untested |
| Scene matrix-column read | Same matrix with stride-three source loads and overlapping output; unmasked exception delivery untested |
| Scene 3x3 transpose | 116,736 exact whole-buffer/status comparisons per Release compiler, raw versus numerical NaN copies, overlapping matrices and all precision/rounding modes; unmasked exception delivery untested |
| Hermite record construction | 1,080,000 exact whole-buffer/status comparisons and null destination per Release compiler; overlaps, NaNs, subnormals and all precision/rounding modes. Unmasked exception delivery is untested. |
| Lua double-index probe | 2,400,000 exact return/index/callback-order and x87-status comparisons, full uint32 range samples, table failure/success, unchanged MXCSR |
| Lua float-index probe | Same matrix, including conversion status at each callback |
| Stored spline negation | 840,000 exact output/status comparisons with NaNs, subnormals, signed zero, infinities, and overlaps; 504 unmasked invalid/denormal cases comparing exception code, status, and partial output |
| Stored spline doubling | Same 840,000-case masked matrix; 756 unmasked invalid/denormal/overflow cases |
| Float-to-extended storage | 2,520,000 exact binary80/buffer/status cases, special values and byte overlaps; 504 unmasked invalid/denormal cases |
| Numeric float load/store | Same 2,520,000-case encoding/state and overlap matrix; 504 unmasked cases |
| Server float copy | Same numeric load/store matrix, exercising the actual server wrapper; MSVC and Clang Release |
| Wire PutFloat | 720,000 exact return/cursor/error/pointer/buffer/state comparisons; 14,400 unmasked cases including failed preflight, sticky errors, unsigned cursor wrap, aliases and unaligned outputs |

Each matrix exercises all three x87 precision settings and all four rounding
directions. Tests are deterministic finite samples plus explicit boundary cases;
they are not exhaustive proofs. The x86-only reference/measurement assembly stays
in tests and is not a production dependency. Native ARM behavior still needs
verification once an ARM build is available.

## Findings retained from rejected candidates

A doubling candidate using volatile double intermediates matched all sampled
output buffers, but Clang Debug/Release and MSVC Debug had 1,704 masked x87 status
differences. Clang also differed in 60 of 756 unmasked cases: an extra float
temporary reload delivered an overflow trap earlier than the original, which
can return with that exception pending. Disabling precise FP did not fix it.
Direct float additions resolved those differences; the verified production
replacement uses that simpler expression. No tolerance was accepted.

`spline_negate_stored.cpp` supplies a shared unary-kernel test harness; the
doubling test selects its original operation with a test-only macro.

## Open investigation: stored spline addition

```powershell
.\scripts\test-portable-kernel.ps1 -Test spline_add_stored -ContinueOnFailure
```

To reproduce the known Debug failure, add `-Configuration debug`.
This comparison fails on the current Clang Debug toolchain.
Out of 1,680,000 masked cases it has 956 output mismatches (all NaN payload
differences, no mismatches involving a non-NaN value) and 2,390 x87 status
differences. It also has 468 mismatches in 3,024 unmasked cases. The other
three compiler/configuration combinations pass. Clang Debug loads/quietens
both operands before FADDP, whereas the original uses an FADD memory operand.
Production assembly is retained; no tolerance has been accepted. The candidate
is test-only and excluded from the normal CTest/CI pass gate.

## Strict parameter comparison

Ordinary C++ `<` matched return values but had 752 Clang and 145 MSVC
exception-flag mismatches over 24,576 Release comparisons per compiler.
The production replacement promotes the left operand before inspecting NaN
encodings and explicitly raises FE_INVALID for any NaN, including quiet NaNs.
It passes the same matrix with zero return/exception-flag differences and no
MXCSR changes. x87 condition codes and unmasked trap timing are not compared.

`scene_nonzero_float` also passes 24,576 return/exception-flag comparisons per Release
compiler with no MXCSR changes. Unmasked trap timing and x87 condition codes
are not compared.

`scene_interpolation_complete` also passes 24,576 return/exception-flag comparisons per Release
compiler with no MXCSR changes. Unmasked trap timing and x87 condition codes
are not compared.

`lua_nonzero_double` checks the shared binary64 nonzero kernel used by fog
and IK Lua values: 24,576 exact return/exception-flag comparisons per Release
compiler and no MXCSR changes. Condition codes and unmasked traps are untested.

`double_load_copy` compares the portable binary64 copy against FLD/FSTP:
104,448 exact full-buffer/x87-status/MXCSR cases per Release compiler, including
unaligned overlaps, signaling NaNs, subnormals and all precision/rounding modes.
Unmasked exception delivery is untested.

## RPC byte expansion

`rpc_expand127`, `rpc_expand255` and `rpc_expand_comm` compare every byte at
all precision/rounding settings: 3,072 cases per Release compiler and scale.
Output bytes and all x87 status bits except C1 agree, with no MXCSR changes.
Clang has respectively 746, 1,018 and 810 C1 differences; MSVC has none.
C1 is a recorded diagnostic, not a pass condition: a source audit of FNSTSW
consumers and status APIs found no reader of this rounding-history bit.
Numeric results and exception flags remain exact. Unmasked trap timing is
untested. The simple multiplication replaces the failed attempt to preserve C1
with an integer publication temporary. The signed 127-scale result also covers
RpcUpdateExpand, which uses the same FILD/FMUL/FSTP operation.

`rpc_fire_expand_small` and `rpc_fire_expand_medium` exhaust signed byte and
short encodings for weapon decoding: 3,072 and 786,432 cases per Release
compiler, with exact outputs/status outside diagnostic C1 and unchanged MXCSR.
Unmasked trap timing remains untested. The shared helper is named ExpandInteger
to cover both byte and short inputs.

`truncate_double` covers legacy truncating binary64-to-int64 conversion,
including NaN/overflow integer-indefinite results and fractional inexact flags.
49,152 exact result/exception-flag comparisons per Release compiler pass with
unchanged x87 control and MXCSR. Condition codes and unmasked timing are untested.

`rpc_quantize127` and `rpc_quantize255` test float scaling followed by legacy
truncation: 49,152 exact int64-result/exception-flag comparisons per Release
compiler and scale, including fractional and overflow boundaries, NaNs and
infinities; x87 control/MXCSR unchanged. Condition codes and unmasked timing
are untested. These integer scales need at most 32 product significand bits.

`rpc_update_quantize` adds the update encoder pointer commit to the 127-scale
fixture: 49,152 exact result/commit/exception-flag comparisons per Release
compiler, null/non-null targets and unchanged x87 control/MXCSR. Unmasked trap
timing and condition codes remain untested.

`rpc_fire_quantize_small` and `rpc_fire_quantize_medium` compare the two weapon
compression tiers: 49,152 exact result/exception-flag cases per Release compiler
and tier, with unchanged x87 control/MXCSR. Boundary samples cover the tier
limits, fractional and int64 overflow boundaries, exceptional and random floats.
Power-of-two prescaling is exact and the product fits within 39 significand
bits. Condition codes and unmasked timing remain untested.

`rpc_fire_pick_tier` covers both tier modes: 98,304 exact tier/exception-flag/
stack-balance comparisons per Release compiler, including adjacent values at
64 and 8192, signed zeros, NaNs, infinities and random encodings. Control and
MXCSR remain unchanged; condition codes and unmasked timing are untested.

`material_wire_color` checks chopping during both multiplication and conversion:
49,152 exact result/exception-flag cases per Release compiler, including
subnormals, NaNs and overflow; caller x87 control/MXCSR unchanged. The portable
environment restore replays the input load to retain denormal flags omitted by
MSVC FE_ALL_EXCEPT. Condition codes and unmasked timing remain untested.
