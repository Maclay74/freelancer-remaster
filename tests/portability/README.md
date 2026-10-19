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

## Open investigation: RPC byte expansion

`rpc_expand127`, `rpc_expand255` and `rpc_expand_comm` compare every possible
input byte at all three precision settings and four rounding directions
(3,072 comparisons per Release compiler and scale). All output bits agree.
MSVC also matches x87 status; Clang has 746, 1,018 and 810 status differences
respectively, caused by an extra float reload clearing C1. Publishing via an
integer temporary did not eliminate that reload. Candidates remain test-only,
outside the passing CTest suite; production assembly is unchanged. No tolerance
has been accepted. Use `-ContinueOnFailure` to collect both compiler results.

`texture_frame_copy` checks four UV floats loaded backwards before forward
stores: 55,296 exact buffer/status comparisons per Release compiler, overlapping
buffers and special values. Unmasked exception delivery is untested.
