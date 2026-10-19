"""Inventory source assembly, including inactive compiler branches.

This is a lexical inventory, not a C++ parser. Unknown enclosing scopes remain
visible for review rather than being omitted. Test-only reference assembly is
outside src/ and intentionally excluded.
"""
from pathlib import Path
import argparse
import json
import re
from bisect import bisect_right

ROOT = Path(__file__).resolve().parents[1]
INVENTORY = ROOT / "docs/portability/assembly-inventory.json"
TOKEN = re.compile(r'//[^\n]*|/\*[\s\S]*?\*/|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|\w+|[^\s]')

def scan():
    records = []
    for path in sorted((ROOT / "src").rglob("*")):
        if path.suffix.lower() not in {".cpp", ".c", ".h", ".inl"}:
            continue
        source = path.read_text(encoding="utf-8-sig", errors="replace")
        if not re.search(r'\b(?:__asm|_asm)\b', source): continue
        newlines = [m.start() for m in re.finditer("\n", source)]
        tokens = [(m.group(), bisect_right(newlines, m.start()) + 1)
                  for m in TOKEN.finditer(source) if not m.group().startswith(("//", "/*", '"', "'"))]
        parens, scopes = [], []
        closing = {}
        for i, (token, line) in enumerate(tokens):
            if token == "(": parens.append(i)
            elif token == ")" and parens: closing[i] = parens.pop()
            elif token == "{":
                j = i - 1
                while j >= 0 and tokens[j][0] in {"const", "override", "final"}: j -= 1
                if j in closing and tokens[closing[j] - 1][0] == "noexcept":
                    j = closing[j] - 2
                name = None
                if j in closing:
                    opening = closing[j]
                    candidate = tokens[opening - 1][0]
                    if candidate not in {"if", "for", "while", "switch", "catch", "noexcept", "__declspec"}:
                        name = candidate
                        if candidate == "FAST_DOT_ENTRY": name = tokens[opening + 1][0]
                        # Include explicit class/namespace qualification.
                        k = opening - 2
                        while k >= 2 and [tokens[k - 1][0], tokens[k][0]] == [":", ":"]:
                            name = tokens[k - 2][0] + "::" + name
                            k -= 3
                scopes.append((name, line))
            elif token == "}" and scopes: scopes.pop()
            if token not in {"__asm", "_asm"}: continue
            owner = next(((name, start) for name, start in reversed(scopes) if name), ("<unknown>", line))
            relative = path.relative_to(ROOT).as_posix()
            records.append(dict(file=relative, method=owner[0], method_line=owner[1], asm_line=line))
    return records

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--refresh", action="store_true")
    args = parser.parse_args()
    current = scan()
    previous = json.loads(INVENTORY.read_text()) if INVENTORY.exists() else {"methods": []}
    methods = {(r["file"], r["method"]): r for r in previous["methods"]}
    live = {}
    for block in current:
        key = (block["file"], block["method"])
        if key not in live:
            live[key] = dict(file=key[0], method=key[1], status="pending", asm_lines=[])
        live[key]["asm_lines"].append(block["asm_line"])
    for key, record in methods.items():
        if key in live:
            for field in ("status", "evidence", "notes"):
                if field in record: live[key][field] = record[field]
        elif record["status"] == "replaced":
            live[key] = {**record, "asm_lines": []}
        else:
            raise SystemExit(f"Assembly disappeared without a recorded replacement: {key}")
    result = dict(scope="All inline assembly under src/, including inactive branches; test references excluded",
                  remaining_blocks=len(current), methods=list(live.values()))
    if args.refresh:
        INVENTORY.parent.mkdir(parents=True, exist_ok=True)
        INVENTORY.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    elif result != previous:
        raise SystemExit("Assembly inventory is stale. Run scripts/assembly_inventory.py --refresh and review it.")
    print(f"{len(current)} blocks; {sum(r['status'] != 'replaced' for r in live.values())} methods pending; "
          f"{sum(r['status'] == 'replaced' for r in live.values())} methods replaced")
    for record in live.values():
        if record["method"] == "<unknown>": print(f"Review unknown scope: {record['file']}:{record['asm_lines']}")

if __name__ == "__main__": main()
