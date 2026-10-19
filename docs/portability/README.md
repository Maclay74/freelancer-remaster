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

For each method:

1. Isolate the original instruction sequence as a test-only reference.
2. Implement the actual production C++ kernel so the test exercises it directly.
3. Compare returned values, output bytes, supported aliasing, and relevant
   floating-point state over normal inputs, boundaries, and exceptional values.
4. Run MSVC and clang-cl in x86 Debug and Release, with relevant rounding and
   precision settings. Record compiler/configuration limits explicitly.
5. Investigate mismatches before switching production callers. Exact equality is
   required unless a specific tolerance and its implications are accepted.
6. Update the inventory and commit that method separately.

The light-color experiment in `tests/experiments/` is evidence, not a production
replacement: normal color samples agreed, but Clang Debug at 64-bit x87 precision
changed extended return bits for some full-int32 inputs. Keep that distinction
when considering `ComputeLightColor`.

Completion additionally requires no production assembly dependency, a functional
Windows x86 build, and gameplay smoke tests. Passing isolated kernels alone does
not satisfy those gates. Work remains active until all entries and integration
gates are accounted for.
