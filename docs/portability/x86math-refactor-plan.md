# x86math responsibility breakdown

Baseline review after `f21d00b`. This is the proposed decomposition;
the implementation has not been moved into these files yet.

First simplification implemented at the user's request: the lookup table and its
initializer have been removed. `ScalarMath.h` now implements `Sqrt` with standard
square root and `InvSqrt` with a double square root and reciprocal. The legacy
implementation remains only in the isolated comparison test. The ownership table
below describes the baseline; scalar math no longer needs initialization state.

`src/x86math/x86math.cpp` currently contains 1,510 lines and 33 assembly blocks
in 33 methods, including conditional x86 implementations. Of these, 23 blocks
belong to the remaining FastDot variants. The public interface lives separately
in `src/include/x86math.h`.

## Proposed ownership

| Piece | Owns | Current methods and helpers |
| --- | --- | --- |
| `MathComponent.cpp` | DACOM integration, lifetime, registration, interface forwarding | `DllMain`, singleton, `QueryInterface`, `AddRef`, `Release`, `CreateInstance`, component identity and priority, `CallDacomStricmp` integration |
| `MatrixMath.cpp` | Operations on 3x3 matrices | `Scale`, `Det`, `Inverse`, `MulMatrix`, `TransposeMatrix`, `WriteAdjugate`, singularity threshold |
| `TransformMath.cpp` | Single-vector transforms and affine composition/inversion | `MulTransform`, `TransformXform`, `TransformMatrix`, `Rotate`, `InverseRotate`, `TransposeTransform`, `InverseTransform`, `GeneralInverse`, `MulVecByMatrix`, `MulVecByTranspose` |
| `QuaternionMath.cpp` | Quaternion operations and matrix conversion | `MulQuat`, `TransformQuat`, `MatrixToQuaternion`, `QuaternionToMatrix`, `QuatNlerp`, `QuatNlerpValue` |
| `TransformBatch.cpp` | Applying a matrix or transform to arrays of vectors | `TransformListXform`, `TransformListMatrix`, `TransposeTransformList`, `InverseTransformList`, `TransformListFast`; initially also all FastDot machinery |
| `ScalarMath.cpp` | Square-root algorithms and their initialization | `BuildInvSqrtTable`, `TableInvSqrt`, `InvSqrt`, `Sqrt`, inverse-square-root table |

Keep `I3DMathEngine` and the public data layouts unchanged during extraction.
Keep `FMathEngine` as the adapter implementing that interface; private numerical
helpers should operate on the existing data structures without knowing about
DACOM, module loading, or singleton lifetime. These pieces remain inside the
existing DLL; they do not need separate libraries, services, or public interfaces.

Current batch implementation: `BatchTransform.h` replaces the full FastDot
machinery with one portable loop. Original specializations survive only in the
test oracle. Component tests now exercise public batch operations. The legacy
zero/component helper and its four standalone tests are historical; they are no
longer production code or part of normal regression runs.

## Dependencies

- Component forwards interface calls to numerical pieces and initializes scalar
  state on attachment, preserving the current allocation-failure behavior.
- Transform operations use matrix helpers for adjugates/inversion.
- Bulk transforms use matrix transpose and single-vector transpose multiplication
  when preparing inverse transforms.
- Quaternions consume the existing matrix/vector types. Their normalization is
  implemented directly with FSQRT today, independently of the approximate table.
- Scalar math owns its table; other pieces should not access table bytes.

Avoid introducing a catch-all helper header containing every implementation.
Use private declarations only for the small set of operations crossing these
boundaries. Preserve current arithmetic and store order during file extraction;
moving code out of line can itself change compiler rounding/inlining behavior.

## Candidates for later simplification

1. **FastDot dispatch (done).** `BatchTransform.h` replaces all 27 coefficient
   specializations and saved-operation dispatch with one matrix-vector loop.
   Regression tests cover standard out-of-place math and corrected exact in-place
   use, with explicit scaled tolerances and general zero-term arithmetic.
   Performance has not been measured; this change makes no speed claim.
2. **Table-based inverse square root (done).** `ScalarMath.h` replaces the
   1,024-byte table and Newton steps with standard square-root operations and a
   double reciprocal root. Numerical changes are recorded in the isolated test
   documentation. Quaternion normalization remains separate.
3. **Shared transform formulas.** Matrix transform and rotation already share
   one helper; transpose transform and inverse rotation share another. Refactor
   around those existing responsibilities before adding abstractions. Preserve
   source/destination alias behavior until deliberately changed.
4. **Quaternion naming and implementation (done).** The former `QuatSlerp` and
   `QuatSlerpValue` operations are now `QuatNlerp` and `QuatNlerpValue`, including
   callers and direct wrappers. They retain their virtual slots. The portable
   implementation in `QuaternionMath.h` preserves sign-adjusted linear blending
   and normalization, with explicit numerical tolerances in isolated tests.
   Replacing it with true slerp would change animation behavior.
5. **Unused production helper (removed).** `RecipSqrtFsqrt` had no production
   caller. It and its constant-only test checks are removed; actual scalar and
   quaternion operations have direct coverage.

## Behavioral decisions to keep explicit

- `Inverse` and `GeneralInverse` return an error for determinant magnitude at or
  below `1e-8f`, but still write the unscaled adjugate. `GeneralInverse` also
  writes translation and `W`; preserve those outputs.
- Several operations intentionally read input after writing output. An ordinary
  temporary or an in-place-safe implementation can produce different results
  for aliased arguments.
- FastDot kernels can return an unrounded x87 value, and the translation path
  adds to it before the final float store. A portable function returning `float`
  can introduce an earlier rounding boundary.
- A zero/one specialization can skip arithmetic that a general dot product
  performs. Infinity, NaN, signed zero and exception flags can therefore differ
  even without an extended-precision issue.
- Existing non-x86 fallback expressions are candidate implementations, not
  evidence that they preserve the x86 path.

The current assembly-removal goal requires observable behavior to be preserved.
Do not silently adopt ordinary float results or a tolerance while extracting
files. Any deliberate relaxation for a later algorithm replacement needs a
recorded scope and acceptance decision; keeping every historical optimization
is not itself a goal.

## Refactor sequence and verification

1. Extract component integration and private declarations; group numerical
   code by the ownership table without changing algorithms. Update both CMake
   and the Visual Studio project. Migrate assembly inventory locations rather
   than recording moved blocks as removed.
2. Decouple `tests/build/x86_math_contract.cpp` from including the entire
   implementation `.cpp`; link the numerical sources and include private test
   declarations instead. Keep DLL registration outside numerical tests.
3. Refactor matrix operations, single transforms, quaternions, bulk transforms,
   and scalar approximations separately. Retain the old implementation as a
   test oracle while replacing each method. Prioritize FastDot simplification
   when the required numerical behavior has been established.
4. Run only relevant isolated MSVC/Clang Release comparisons for each change,
   per the current testing preference. Use Debug to investigate specific
   failures, and rebuild the component at structural milestones. Do not rebuild
   or restage the entire game for each method.

The existing component contract covers simple values for all 27 FastDot variants,
two reciprocal square roots, two quaternion-to-matrix examples, and antipodal
quaternion interpolation including two aliases. It does not validate all matrix
operations, singular inverses, general inputs, or broad numerical equivalence.
The isolated zero/component FastDot tests provide additional coverage only for
those four replacements. Neither suite proves that all historical x87 behavior
can be discarded. Run gameplay checks at integration milestones, especially
cutscenes, ship movement, camera transforms, and object animation.

## Assembly removal completed for x86math

`src/x86math` now has zero inline-assembly blocks, naked entry points or FastDot
calling-convention workarounds. The last eight blocks were handled individually:
five matrix/affine kernels, two quaternion conversions and one unused helper.
Numerical implementations live in `MatrixMath.h`, `TransformMath.h`,
`QuaternionMath.h`, `BatchTransform.h` and `ScalarMath.h`; `x86math.cpp` retains
the interface adapter, existing C++ inverse/quaternion operations and DLL setup.
Redundant local forwarding helpers have also been removed.

The DLL builds in the existing Windows x86 Clang Release configuration. The
public component contract covers matrix multiplication, affine composition,
single-vector/inverse transforms, exact in-place operations, conversions,
batches, scalar math and nlerp. Isolated core and conversion checks pass with
MSVC and Clang Release. New tests have explicit tolerances rather than claiming
bit-exact x87 emulation. Original implementations are test-only oracles. The
existing inverse formulas, error codes and determinant threshold were retained.

This completes the module's manual-assembly removal, not its platform port or
all proposed architectural work. The component test still includes the adapter
source directly, and Windows/DACOM ABI dependencies remain. Debug, other
architectures and gameplay smoke tests are not validated here. The whole game
has not been rebuilt or restaged during this module refactor. CI stays disabled.
