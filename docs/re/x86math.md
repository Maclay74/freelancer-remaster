# x86math.dll — 3DMathEngine

`x86math.dll` is a small DACOM module with fast 3D operations (matrices, transforms, quaternions, `InvSqrt`) used by EngBase, Common, RP8, RendComp, Shading, Thorn, alchemy, Deformable2, flmaterials, Content and Server. This document explains how to obtain the object, what types it has, and which numeric peculiarities the port must preserve. The port is `src/x86math/`, the public ABI is [src/include/x86math.h](../../src/include/x86math.h) (`I3DMathEngine`, `FVector`/`FMatrix`/`FTransform`/`FQuaternion`, `FMathDesc`); function slots and statuses live in decomp-tracker (listing `EXE/Disasm/x86math.dll.txt`). Rules for calling foreign interfaces — [cross_dll_protocol.md](cross_dll_protocol.md).

## How to obtain the object

There are no PE exports (only `DllMain`); other modules do not link against x86math but create the object through `ICOManager::CreateInstance` with the descriptor `{size = 0xC, interface_name = "3DMathEngine", implementation = NULL}` (field `+8`: NULL or `"x86"`). The factory is registered with priority `0x40000000`. Codes: success — `GR_OK`, `*out` is the object with AddRef; `desc == 0` or no name — `0xFFFFFFFE`; wrong size/name/implementation — `0xFFFFFFFD`. The name is compared bytewise with `"3DMathEngine"` (13 bytes). `QueryInterface` on the object itself always returns `*out = 0` and `GR_GENERIC`: the IID `"1.11_I3DMathEngine"` is not used in retail, the only path to the object is `CreateInstance`. `AddRef`/`Release` always return 1 (singleton). If `CreateInstance` is broken, EngBase gets null math pointers and crashes on the first call.

`DllMain` (attach): creates the object with two vfptrs (math interface + factory, 8 bytes) and registers the component. If `new` fails it returns TRUE. The port no longer initializes an inverse-square-root table.

## Types

`Vector` — 12 bytes `{X,Y,Z}`; `Matrix` — 36 bytes, row-major `d[row][col]`, `out = M * v` gives `out.x = row0·v`; `Transform` — 48 bytes, `Matrix` + translation at `+0x24`; `Quaternion` — 16 bytes `{W,X,Y,Z}` (scalar first). The slot order of `I3DMathEngine` does not match the Liberty `3dMath.h` reconstruction; the source of truth is the retail vtable. The port added a `QuatNlerpValue` slot with `const float* T` at the end of the interface — a contract between our modules, not retail compatibility (the former `QuatNlerp(float)` delegates to it).

## Retail numerical behavior (historical)

- Retail methods use x87 intermediate precision, early float stores and order-dependent alias behavior. The current port has replaced all inline assembly in `src/x86math` with portable C++ math or removed unused code. Double intermediates and safe in-place operations intentionally differ from exact retail results; numerical test limits and remaining platform dependencies are documented below.
- `MIN_DET = 1e-8f`: when `|det| <= MIN_DET`, `inverse` and `general_inverse` write the adjugate without dividing and return `GR_INVALID_PARMS`.
- **`InvSqrt`/`Sqrt`**: a table with 10 mantissa bits (`0x400` bytes) + two Newton iterations `(3 - y²x) * y * 0.5`; `Sqrt` = the same result × x. In the PE the table is zeros (BSS) and is built in `DllMain` by an x87 generator (`FLD (i|0xFC00)<<14`, `FSQRT`, `FDIVR 1.0`, `(bits+0x2000)>>15`), after which `table[0x200] = 0xFF` (the mantissa slot of 1.0, where the computed byte overflows). Comparing against the PE file is meaningless; compare against the live table after `DllMain`; isolated discrepancies are possible with a different x87 control word.
- **`transform_list` (Matrix)** — "FastDot": each matrix row is classified as `==0` / `==1` / other (27 kernel variants, FPU order: Z·C, then Y·B, then X·A); if the sum of "saved operations" over the three rows is `< 4`, the common triple MAD is used, otherwise three indirect kernel calls. Not SIMD. The `Transform` variant adds the translation on x87 before the final `float` store (with zero translation — a tail call into the Matrix version).
- **`quat_slerp`** — not Shoemake slerp but nlerp with normalization: for `dot >= 0` lerp `A + T*(B-A)`, otherwise `A - T*(A+B)` (lerp towards `-B`); normalization via `FSQRT` + `FDIVR 1.0`, **not** via the table-based `InvSqrt`; zero length is not checked.
- `matrix_to_quaternion` / `quaternion_to_matrix` — standard formulas (trace / largest diagonal; for `matrix_to_quaternion` the branch is chosen by an x87 comparison, including unordered), with rounding to `float` only on the result writes.

## Port deviations

1. No CRT `_initterm`/onexit or inverse-square-root table initialization. At the user's request, `Sqrt` now uses standard square root and `InvSqrt` uses `1.0 / sqrt(double(X))` narrowed to float (`src/x86math/ScalarMath.h`). This deliberately changes approximation/edge-case results and floating-point flags. The table algorithm described above documents retail behavior; it is no longer used in production.
2. The original FastDot specialization machinery has been replaced by the portable batch loop in `BatchTransform.h`. All matrix terms are evaluated; XYZ is read before output stores, allowing exact in-place use. Matrix/translation values are snapshotted for the call. This deliberately changes legacy sparse-path in-place behavior, zero-term exceptional arithmetic, and low-order rounding. Slots 16/17 remain `DACOM_API` stdcall. The 27 kernels described above document retail behavior and are no longer used in production.
3. The object is created with `::operator new(8, nothrow)` + placement new (MSVC writes the vfptr itself, retail does it manually; `sizeof == 8`).
4. The port's `QueryInterface` and `CreateInstance` check the output pointer for NULL; retail writes without a check.
5. The math implementations use portable C++, but the DACOM/Windows DLL wrapper and current x86 ABI still need platform work; removing manual assembly does not make the whole module cross-platform.

6. The former `QuatSlerp`/`QuatSlerpValue` methods and their callers are named
   `QuatNlerp`/`QuatNlerpValue` in the port, with virtual slots and calling
   conventions unchanged. `src/x86math/QuaternionMath.h` implements the same
   sign-adjusted linear blend and normalization in C++, using double
   intermediates and standard square root. Component stores and live per-lane
   blend-factor reads retain alias behavior. There is still no zero-length
   fallback. Low-order rounding, NaN payload/sign and exception flags may differ
   from the original assembly; isolated tests record a 2e-6 component tolerance.

7. `MatrixMath.h` and `TransformMath.h` now own matrix multiplication, transpose,
   single-vector application and affine composition. They share the portable
   batch kernel and compute into temporaries for safe exact in-place use.
   Quaternion conversion uses standard trace/largest-diagonal and normalized
   matrix formulas in `QuaternionMath.h`, preserving W/X/Y/Z and row-major
   conventions. The unused `RecipSqrtFsqrt` helper is removed. Original assembly
   survives only as test oracles. Public method slots, result codes, singular
   inverse threshold and component registration are unchanged. The x86math DLL
   build and public component contract pass; gameplay smoke tests remain pending.
