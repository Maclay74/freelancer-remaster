# Portable extended-arithmetic feasibility prototype

```powershell
.\tests\experiments\softfloat\compare.ps1
```

This standalone experiment downloads Berkeley SoftFloat to ignored
`out/experiments/softfloat/dependency`, pinned to upstream commit
`a0c6494cdc11865811dec815d5c0049fba9d82a8`. Git, LLVM, and Visual Studio C++ x86
tools/Windows SDK are required. No game, DATA, or legacy SDK is built or needed.
The normal production build does not fetch or link SoftFloat.

SoftFloat provides software binary80 arithmetic and reduced rounding precision;
its `8086` specialization models x87 NaN propagation. See the upstream
[interface](https://www.jhauser.us/arithmetic/SoftFloat-3/doc/SoftFloat.html) and
[specialization documentation](https://www.jhauser.us/arithmetic/SoftFloat-3/doc/SoftFloat-source.html).
Upstream source and its notices/COPYING.txt remain intact in the checkout. Only
our driver/configuration are stored here; no upstream sources are vendored.

The x86 reference loads binary32 A and applies addition, subtraction,
multiplication, division, or square root. Each result is observed both in full
binary80 precision and after a binary32 store. The software path quietens A
during conversion but preserves the signaling state of memory operand B until
arithmetic. Quietening both operands first would repeat the NaN-payload bug
found in direct Clang Debug C++ addition.

All four local MSVC/Clang Debug/Release comparisons passed: 1,200,000 input
pairs per operation per configuration (24 million cases in total), all three
precision settings/four rounding directions, including NaN pairs, infinities,
signed zero, and subnormals. Binary32 results, full binary80 result bits, and
the five standard exception flags matched exactly.

**This does not qualify a production replacement.** It excludes x87's denormal
operand flag, C1/other condition bits, host exception-state updates, deferred
unmasked traps, partial output, callback behavior, and performance. Those gaps
must be investigated before integrating a software backend. Inputs are widened
binary32 values; arbitrary binary80 inputs/intermediate chains also still need
testing. No numeric
tolerance has been accepted, and the failing stored-addition method remains
unchanged in production.
