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

## Rejected candidate: stored spline addition

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
This historical candidate is test-only and excluded from the normal CTest gate.
The replacement below explicitly preserves NaN selection instead; no numerical
tolerance has been accepted.

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

`light_color_store` and `scene_ambient_store` compare their distinct float-to-int
triplet publication orders: 43,008 exact buffer/exception-flag/stack-balance
cases per Release compiler and method, including overlapping buffers, special
values and all precision/rounding settings; MXCSR unchanged. Condition codes
and unmasked timing are untested.

`lua_vector_copy` checks numerical X/Z and raw Y with X/Z/Y publication:
61,440 exact buffer/exception-flag/stack-balance cases per Release compiler,
overlapping buffers and explicit narrowing underflow/overflow boundaries.
Control/MXCSR are unchanged. Environment restoration plus original-input
reloads remove a Clang-only denormal flag from reloading a narrowed result.
Condition codes and unmasked timing remain untested.

`fast_dot_zero` and `fast_dot_x/y/z` are now historical, standalone checks,
excluded from normal CTest/CI lists. Their helper lives under `candidates/`,
with no production dependency. Their earlier evidence for zero/component kernels:
12,288 exact binary80 return and complete x87-status comparisons per Release
compiler and variant, all precision/rounding settings, special/random float
encodings, unchanged control/MXCSR and fastcall stack cleanup. Zero is tested
with a null vector. The other dot kernels retain unrounded arithmetic and are
not covered by these four replacements. Unmasked timing remains untested.

`spline_scale_stored` checks reverse-order vector scaling: 43,008 exact
buffer/exception-flag/stack-balance cases per Release compiler, overlapping
buffers and all special-value pairs. Caller control/MXCSR are unchanged.
Explicit NaN selection preserves source-load-before-factor semantics; a volatile
factor read prevents Clang from adding denormal flags on a source-NaN path.
Condition codes and unmasked timing remain untested.

`rpc_add_item_fields` and `rpc_modify_item_fields` check the health load, ID/
count/mounted writes and final health store: 12,288 exact field-buffer/exception-
flag/stack-balance cases per Release compiler and variant, all mounted bytes,
modify-ID zero extension and special/random health encodings; control/MXCSR
unchanged. The production mounted packet field remains written after this
helper returns. Condition codes and unmasked timing are untested.

`lua_constant_compare` checks Lua constant equality-or-unordered lookup:
24,576 exact boolean/exception-flag/stack-balance comparisons per Release
compiler, special pairs and random equal/adjacent/unrelated doubles. MXCSR is
unchanged. The helper remains C-compatible for the existing parser build.
Condition codes and unmasked timing are untested.

`lua_number_classify` checks opcode classification before constant lookup:
49,152 exact emission-decision/argument/sign and exception-flag/stack-balance
cases per Release compiler. Covers integer limits, fractions, zeros, infinities,
NaNs and random doubles; control/MXCSR unchanged. NaNs retain the recovered
negative opcode with zero argument; rejected fractions retain FE_INEXACT.
Condition codes and unmasked timing remain untested.

`spline_add_portable` and `spline_subtract_portable` compare reverse-order
binary vector operations: 86,016 exact whole-buffer/exception-flag/stack-balance
cases per Release compiler and operation, including overlaps with either input,
all precision/rounding modes, special-value pairs and random encodings.
Caller control/MXCSR are unchanged. The shared NaN helper preserves the original
left numerical load and raw right memory operand; invalid infinity combinations
produce the original negative quiet NaN. Scaling also passes its 43,008-case
regression matrix after sharing that helper. Debug, condition codes and unmasked
trap timing remain untested for these new implementations.

## Standard square root / inverse square root

`scalar_math` checks the intentional replacement of the old x86math table and
Newton approximation with standard math. Run:

```powershell
.\scripts\test-portable-kernel.ps1 -Test scalar_math
```

MSVC and Clang Release each pass 196,608 input pairs across all precision and
rounding settings: normal exponent boundaries, random encodings, subnormals,
signed zero, infinities, negative values, and quiet/signaling NaNs. For positive
finite inputs the squared relative residuals `abs(root*root/x - 1)` and
`abs(inverse*inverse*x - 1)` must be <= 5e-7. Observed maxima are below 2.94e-7.
The test verifies signed-zero/infinity/NaN classifications and unchanged x87 and
MXCSR control settings. It does not require matching old flags, NaN payloads,
errno, or unmasked exception delivery, and does not test Debug.

The retained legacy initializer still uses its original assembly; both old and
new algorithms are evaluated by each compiler. Diagnostic mismatch counts are
not failures: bit equality with the approximation is intentionally not the
contract. Largest sampled inverse relative change for normal inputs is below
2.48e-7, while the smallest positive subnormal differs by about 99.89% relative
to the new result. These samples do not establish an exhaustive error bound.
The component contract also checks both public API wrappers and `GR_OK` results
for four and zero. No game build or gameplay test is performed by this fixture.

## Quaternion normalized linear interpolation

```powershell
.\scripts\test-portable-kernel.ps1 -Test quat_nlerp
```

`quat_nlerp` exercises the production template from `QuaternionMath.h` against
retained original assembly: 172,032 cases per MSVC/Clang Release compiler, all
three precision settings and four rounding modes. Cases include random unit
quaternion pairs; endpoints and extrapolation; identical, antipodal, orthogonal
and near-orthogonal pairs; zero/tiny/large inputs; NaNs/infinity; both full input
aliases and a blend factor aliasing each output lane. Output remains normalized
when finite; the original zero-length behavior is retained, without a fallback.

This is an algorithm-preserving numerical rewrite, not an x87 emulator. Absolute
component difference and squared-length error must each be <= 2e-6. Observed
component maximum: zero for Clang finite outputs, 1.78813934e-7 for MSVC;
squared-length error maximum: 4.99358116e-7. NaNs are compared by classification;
payload, sign, signed-zero bit equality and old exception flags are not required.
The test logs buffer/flag differences as diagnostics and verifies unchanged
floating-point control settings, stack balance and untouched input objects.
Debug, unmasked exception timing and non-x86 builds remain untested.

The public methods and direct wrappers/callers are now `QuatNlerp` and
`QuatNlerpValue`; virtual slot order and calling conventions are unchanged.

## Standard batch transforms

```powershell
.\scripts\test-portable-kernel.ps1 -Test batch_transform
```

The production `BatchTransform.h` template replaces all 27 FastDot variants and
both bulk dispatch paths. It snapshots matrix/translation once and XYZ per
vector, evaluates full dot products using double intermediates, then narrows at
output stores. Positive counts support separate buffers and exact in-place use.
Zero/negative counts do nothing, including when source/destination are null.
Shifted partially overlapping arrays are not covered by the supported in-place
contract; forward iteration can overwrite subsequent source elements.

MSVC and Clang Release each pass 49,152 batch cases and 24,576 comparisons with
the original out-of-place batch path. Coverage includes all 27 row coefficient
classes, random dense matrices, identity/zero/rotation matrices, counts -1/0/1/8,
no/zero/nonzero/output-aliased translation, separate and exact in-place buffers,
and all precision/rounding settings. An independent row/column model uses a
2e-6 scaled tolerance: absolute error divided by the maximum of one and the
sum of absolute input terms plus translation. Observed maximum scaled errors:
3.43e-7 against the model and 1.90e-7 against legacy. Untouched array regions,
control settings and stack balance are checked.

The old sparse path reread source coordinates after writing output; it differs
from the mathematical in-place result in 2,676 sampled batches per compiler.
The new loop intentionally corrects this. Matrix and translation values are
fixed for the call rather than depending on destination writes.

Full arithmetic intentionally evaluates zero coefficients, so zero times infinity
or NaN can propagate NaN where a specialized kernel previously skipped a term.
An identity/infinite-source case checks this policy. Old exception flags, NaN
payload/sign and signed-zero bits are not preserved. Debug, shifted overlap,
performance comparisons, other architectures and gameplay remain untested.
The public component contract tests rotation in place and translated batches.

## Core matrix and affine math

`x86_core_math` compares the portable kernels in `MatrixMath.h` and
`TransformMath.h` with five original x87 operations: matrix/vector multiplication,
transpose/vector multiplication, affine application, matrix multiplication and
affine composition. MSVC/Clang Release each pass 6,144 cases per operation across
all precision/rounding settings, deterministic identity/zero/rotation/scale cases
and random finite matrices/vectors. A 2e-6 scaled tolerance uses the maximum of
one and the sum of absolute input terms; observed error is below 1.883e-7.
Additional checks validate exact in-place inputs, both composition/multiplication
operand aliases, in-place transpose, control settings and stack balance. Original
in-place overwrite behavior is intentionally replaced by mathematical in-place
results. Flags, NaN payload/sign and signed-zero bits are not required to match;
Debug, arbitrary shifted overlap, other architectures and gameplay are untested.

## Quaternion/matrix conversions

`quat_conversion` checks both production conversion templates against retained
assembly, plus matrix/quaternion/matrix round trips. MSVC and Clang Release each
pass 6,144 pairs across all precision/rounding settings, all trace/diagonal
branches, identity, half-turns about each axis, diagonal ties, random non-unit
quaternions and magnitude scales 1e-12 to 1e12. Absolute component and round-trip
tolerances are 2e-6. Observed maxima: ToMatrix 2.98023224e-7, FromMatrix
1.1920929e-7, round-trip 8.34465027e-7. Zero quaternion input still produces NaNs,
with no invented identity fallback. Control settings and stack balance match;
NaN payload/sign, flags, signed-zero bits, Debug, malformed/nonfinite matrices,
cross-type overlapping storage, other architectures and gameplay are untested.
The quaternion-to-matrix factor remains 2 / squared norm; it does not assume
unit input. Trace/largest-diagonal selection and W/X/Y/Z convention are retained.
