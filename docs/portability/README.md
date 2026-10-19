# Removing assembly dependencies

The goal is a portable C++ implementation for every assembly-dependent method,
with verified observable behavior. This is separate from porting Windows APIs,
graphics backends, pointer-size assumptions, and the rest of the platform layer.

`assembly-inventory.json` tracks the entire source tree, including inactive
compiler branches. The starting inventory is 231 assembly blocks in 193 lexical
method entries across 28 files. Multiple blocks and compiler variants can belong
to one method; this is not a count of independent algorithms. Reference assembly
in tests is intentionally retained as an x86 oracle, not a production dependency.

Refresh or validate the inventory with the configured Python interpreter:

```powershell
python scripts/assembly_inventory.py --refresh
python scripts/assembly_inventory.py
```

The scanner is lexical; review each entry against its source before replacing it.
It preserves replacement records and requires removed entries to be accounted
for. Status is `pending` until the replacement has passed its isolated checks;
use `investigating` when a candidate has a known mismatch. A `replaced` entry must
identify test evidence and exactness/tolerance decisions. No tolerances have been
accepted by default.

`removed-unverified` records assembly removed outside this verification workflow:
the source no longer contains it, but isolated equivalence evidence is still
missing. Such entries remain unfinished work and do not count as verified
replacements. `accepted-unverified` records an explicit user decision to skip
that verification. The twelve removals in user commit `f21d00b` have this status
at the user's request: they are excluded from remaining verification work and
are not counted as tested replacements.

The user requested replacing the x86math inverse-square-root approximation with
standard math. This is a deliberate algorithm change rather than a bit-exact
translation: `Sqrt` uses standard square root and `InvSqrt` uses a double
square root and reciprocal before narrowing. The table and its assembly-based
initializer are gone. See `tests/portability/scalar_math.cpp` for retained legacy
comparisons and the new mathematical contract. Legacy NaN payloads, exception
flags, and edge-case results are not preserved for these two operations.

For each method:

1. Isolate the original instruction sequence as a test-only reference.
2. Implement the actual production C++ kernel so the test exercises it directly.
3. Compare returned values, output bytes, supported aliasing, and relevant
   floating-point state over normal inputs, boundaries, and exceptional values.
4. Run the isolated test with MSVC and clang-cl in x86 Release. Use Debug only
   to investigate a specific failure. Record compiler/configuration limits
   explicitly; passing Release does not establish Debug equivalence.
5. Investigate mismatches before switching production callers. Exact equality is
   required unless a specific tolerance and its implications are accepted.
6. Update the inventory and commit that method separately.

Keep routine work narrow: run the changed kernel once per compiler, retain
deterministic boundary and representative input checks, and reuse evidence for
unchanged shared helpers. Do not rerun the entire suite or rebuild the game for
each method. Build affected targets only when integration changes justify it;
reserve full builds and gameplay checks for integration milestones. Broader
floating-point or dependency experiments require a concrete failure to resolve.

The light-color experiment in `tests/experiments/` is evidence, not a production
replacement: normal color samples agreed, but Clang Debug at 64-bit x87 precision
changed extended return bits for some full-int32 inputs. Keep that distinction
when considering `ComputeLightColor`.

An opt-in software extended-arithmetic feasibility experiment is in
`tests/experiments/softfloat/`. It checks a pinned upstream implementation
against the hardware oracle without adding a production dependency. Numeric
agreement there is not sufficient to switch methods that depend on exception
timing or host floating-point state.

Completion additionally requires no production assembly dependency, a functional
Windows x86 build, and gameplay smoke tests. Passing isolated kernels alone does
not satisfy those gates. Work remains active until all entries and integration
gates are accounted for.

For byte expansion, C1 rounding-history differences are diagnostic only. The
source audit found FNSTSW consumers test AH masks 01/05/41/44 (or equivalent
AX masks), not C1; the two _clearfp calls discard their return values. Outputs,
exception flags and all other tested status bits still require exact equality.
This excludes an unused machine condition bit, not numerical differences.
