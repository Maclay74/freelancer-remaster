# Isolated production-kernel comparisons

No game build, DATA, DirectX SDK, or MFC is needed:

```powershell
.\scripts\test-portable-kernel.ps1 -Test hermite_assignment
.\scripts\test-portable-kernel.ps1 -Test lua_double_probe
.\scripts\test-portable-kernel.ps1 -Test lua_float_probe
.\scripts\test-portable-kernel.ps1 -Test spline_negate_stored
.\scripts\test-portable-kernel.ps1 -Test spline_double_stored
.\scripts\test-portable-kernel.ps1 -Test float_extended_storage
.\scripts\test-portable-kernel.ps1 -Test float_load_copy
.\scripts\test-portable-kernel.ps1 -Test wire_put_float
```

Each command compiles only that standalone test with MSVC and clang-cl, x86
Debug and Release. The test includes the actual production C++ kernel and a
retained assembly oracle. Results go to `out/tests/portability/<test>/results.txt`.
The verified tests also run through CTest and a separate Windows CI job
that does not require the proprietary SDK secret or game DATA.

| Replacement | Comparisons per compiler/configuration |
| --- | --- |
| Hermite record assignment | 1,080,000 exact whole-buffer comparisons, special bit patterns and overlaps; unchanged x87 status/MXCSR |
| Lua double-index probe | 2,400,000 exact return/index/callback-order and x87-status comparisons, full uint32 range samples, table failure/success, unchanged MXCSR |
| Lua float-index probe | Same matrix, including conversion status at each callback |
| Stored spline negation | 840,000 exact output/status comparisons with NaNs, subnormals, signed zero, infinities, and overlaps; 504 unmasked invalid/denormal cases comparing exception code, status, and partial output |
| Stored spline doubling | Same 840,000-case masked matrix; 756 unmasked invalid/denormal/overflow cases |
| Float-to-extended storage | 2,520,000 exact binary80/buffer/status cases, special values and byte overlaps; 504 unmasked invalid/denormal cases |
| Numeric float load/store | Same 2,520,000-case encoding/state and overlap matrix; 504 unmasked cases |
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

This comparison intentionally fails on the current Clang Debug toolchain.
Out of 1,680,000 masked cases it has 956 output mismatches (all NaN payload
differences, no mismatches involving a non-NaN value) and 2,390 x87 status
differences. It also has 468 mismatches in 3,024 unmasked cases. The other
three compiler/configuration combinations pass. Clang Debug loads/quietens
both operands before FADDP, whereas the original uses an FADD memory operand.
Production assembly is retained; no tolerance has been accepted. The candidate
is test-only and excluded from the normal CTest/CI pass gate.
