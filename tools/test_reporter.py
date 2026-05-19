#!/usr/bin/env python3
"""
Runs a language binding test, streams its output, parses pass/fail counts,
and writes the results to a file for the final summary.

Usage:
    python3 test_reporter.py --binding NAME --results-file PATH -- CMD [ARGS...]
"""
import argparse
import re
import subprocess
import sys


def _parse(binding, text):
    b = binding.lower()
    if b == "core":
        # TAP format (GLib 2.38+ default): "ok N /path" / "not ok N /path"
        tap_ok = len(re.findall(r"^ok \d+", text, re.MULTILINE))
        tap_fail = len(re.findall(r"^not ok \d+", text, re.MULTILINE))
        if tap_ok + tap_fail > 0:
            return tap_ok, tap_fail
        # GLib non-TAP summary: "OK, N passed; N skipped" or "FAIL, N passed, N failed; ..."
        m = re.search(r"(?:OK|FAIL),\s*(\d+) (?:tests )?passed(?:.*?(\d+) (?:tests )?failed)?", text, re.DOTALL)
        if m:
            return int(m.group(1)), int(m.group(2) or 0)
        # Last resort: count individual result lines
        ok = len(re.findall(r":\s+OK\b", text))
        fail = len(re.findall(r":\s+FAIL\b", text))
        return ok, fail
    elif b == "python":
        m = re.search(r"Ran (\d+) test", text)
        n = int(m.group(1)) if m else 0
        failures = sum(
            int(m2.group(1))
            for pat in (r"failures=(\d+)", r"errors=(\d+)")
            for m2 in (re.search(pat, text),)
            if m2
        )
        return n - failures, failures
    elif b == "java":
        m = re.search(r"Results: (\d+) passed, (\d+) failed", text)
        return (int(m.group(1)), int(m.group(2))) if m else (0, 0)
    elif b in ("c++", "cpp"):
        mp = re.search(r"\[  PASSED  \] (\d+) test", text)
        mf = re.search(r"\[  FAILED  \] (\d+) test", text)
        return (int(mp.group(1)) if mp else 0, int(mf.group(1)) if mf else 0)
    elif b == "go":
        passed = len(re.findall(r"^--- PASS:", text, re.MULTILINE))
        failed = len(re.findall(r"^--- FAIL:", text, re.MULTILINE))
        return passed, failed
    return 0, 0


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--binding", required=True, help="Display name for this binding")
    ap.add_argument("--results-file", required=True, help="Path to write pass/fail counts")
    ap.add_argument("cmd", nargs=argparse.REMAINDER)
    args = ap.parse_args()

    cmd = args.cmd[1:] if args.cmd and args.cmd[0] == "--" else args.cmd
    if not cmd:
        print("test_reporter: no command given", file=sys.stderr)
        sys.exit(1)

    if args.binding.lower() == "core":
        header = "── Core Tests ──"
    else:
        header = f"── {args.binding} Bindings ──"
    print(f"\n\033[1m{header}\033[0m\n", flush=True)

    lines = []
    proc = subprocess.Popen(
        cmd,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
    )
    for line in proc.stdout:
        sys.stdout.write(line)
        sys.stdout.flush()
        lines.append(line)
    proc.wait()

    output = "".join(lines)
    passed, failed = _parse(args.binding, output)

    with open(args.results_file, "w") as f:
        f.write(f"{args.binding} {passed} {failed}\n")

    sys.exit(proc.returncode)


if __name__ == "__main__":
    main()