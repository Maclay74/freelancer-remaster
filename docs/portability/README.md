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

Trigonometry now uses `<cmath>` directly at all 140 former wrapper call sites
across 26 Common/IVP files. The production `x87_trig.h` has been deleted; its
instruction implementations survive only as a test oracle. Double arguments
and explicit float-result conversions are retained, but standard range reduction
and rounding replace the original CPU instructions. This is a numerical
modernization: exact bits, x87 flags, NaN payloads, and behavior at tangent poles
or extreme angles are not legacy contracts. AI and physics trajectories may
therefore differ slightly. Cross-platform deterministic simulation is not proved.
`standard_trig` compares all ten former wrapper expressions with the original
instructions in MSVC/clang-cl Release, using 24/53/64-bit x87 precision with
round-to-nearest. See the test README for domains and tolerances. The Common DLL
and compiler/resource contract also build and pass their targeted checks; no
gameplay run, staging, Debug pass, or non-x86 validation was performed.

Texture playback has also moved to standard C++ in
`src/Shading/TexturePlaybackMath.h`: duration calculation and clamp/loop/ping-pong
advancement retain the 0.001f conversion, 1e-6 duration adjustment, strict
reflection conditions and final float-time publication. Cached frame-count
handling and the existing elapsed/frame setter stay in TextureLibrary.cpp.
The eight assembly blocks in Shading are gone. Isolated Release tests compare
the original assembly with the production math on MSVC and Clang, including
frame/direction results and playback sequences. Double/float rounding can differ
from retained x87 arithmetic; numerical tolerances are explicit in the test
README. This does not claim bit-identical timing at every possible boundary,
identical exception flags, or cross-platform deterministic animation.

Lua's nine remaining math-library instruction sequences now use C-compatible
standard math in `src/Thorn/lua32/src/lib/PortableMath.h`. The script API keeps
degree units, original conversion constants, and argument-check order. Like the
shared trigonometry migration, this deliberately adopts standard range reduction
and domain behavior rather than emulating x87 exceptional intermediates. The
isolated comparison and actual C callback contract are described in the test
README; they do not prove full Lua VM, cutscene or gameplay equivalence.

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
