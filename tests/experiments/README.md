# Isolated assembly versus C++ comparison

From the repository root:

```powershell
.\tests\experiments\compare-light-color.ps1
```

This compiles **only** `light_color_comparison.cpp`, using clang-cl and MSVC
in x86 Debug (`/Od`) and Release (`/O2`) configurations. It does not invoke
CMake, build the game, launch the game, or require DATA/DirectX/MFC.
It requires the installed LLVM compiler, Visual Studio C++ x86 tools, and
Windows SDK. Outputs go to ignored `out/experiments/light-color/`;
`results.txt` contains the complete comparison.

The function is the `ComputeLightColor` kernel from `src/Thorn/ThornScene.cpp`:
`From + (To - From) * U`. Both implementations preserve its wrapped 32-bit
integer subtraction. The assembly version retains the production instruction
sequence and explicit x87 return handoff. The other version uses ordinary C++
double expressions. Production code is unchanged by this experiment.

For each compiler/configuration and x87 precision setting (24-, 53-, and
64-bit significands), a fixed seed generates 100,000 normal color samples
(From/To in 0..255, U in 0..1) and 100,000 full-int32 stress samples. Normal
color samples include decreasing as well as increasing values. The midpoint
20 -> 220 at U=0.5 must return 120 in both versions.

Each sample is compared twice: after storing the return as a 64-bit double,
and by capturing the raw 80-bit x87 return before a C++ assignment can round
it. The measurement bridge uses an assembly call solely to observe that ABI
register; the C++ calculation itself contains no assembly. Differences are
reported rather than treated as test failures: agreement is a valid finding.

## Observed results on the current toolchain

Across four compiler/configuration combinations and three precision settings:

| Inputs / observation | Result |
| --- | --- |
| Normal colors, stored double and raw return | All 1,200,000 comparisons matched |
| Full-int32 stress, stored double | All 1,200,000 comparisons matched |
| Full-int32 stress, raw return | 33,791 differences in Clang Debug at 64-bit precision; all other settings matched |

Example: From=-1762442959, To=611767611, U=0.908206165 produced raw returns
`c01ed106d5aab4820000` (assembly) and `c01ed106d5aab4820200` (C++).
They round to the same double. These are tiny precision differences, distinct
from the earlier missing-return bug that produced garbage values.

This finite-input experiment does not prove equivalence for every possible
input, unmasked floating-point exceptions, NaN, infinity, other compiler
options, or downstream calculations that retain the extended return. In
particular, these results do not establish that assembly is necessary for
ordinary color interpolation.
