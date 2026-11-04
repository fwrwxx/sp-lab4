#!/usr/bin/env python3
import re, sys

STAT_RE = re.compile(r"STAT name=(\S+) n=(\d+) min_us=(\d+) avg_us=(\d+) max_us=(\d+)")
JIT_RE  = re.compile(r"IMU jitter_min_us=(-?\d+) jitter_max_us=(-?\d+) missed=(\d+)")

def parse(path):
    rows, jitter = {}, None
    for line in open(path, encoding="utf-8", errors="replace"):
        m = STAT_RE.search(line)
        if m:
            rows[m.group(1)] = dict(n=int(m.group(2)), min=int(m.group(3)),
                                    avg=int(m.group(4)), max=int(m.group(5)))
        m = JIT_RE.search(line)
        if m:
            jitter = dict(min=int(m.group(1)), max=int(m.group(2)), missed=int(m.group(3)))
    return rows, jitter

if __name__ == "__main__":
    path = sys.argv[1] if len(sys.argv) > 1 else "-"
    rows, jitter = parse(path)
    print(f"# {path}")
    print(f"{'task':14s} {'n':>8s} {'min_us':>8s} {'avg_us':>8s} {'max_us':>8s}")
    for name, r in rows.items():
        print(f"{name:14s} {r['n']:8d} {r['min']:8d} {r['avg']:8d} {r['max']:8d}")
    if jitter:
        print(f"IMU jitter: min={jitter['min']} us, max={jitter['max']} us, missed={jitter['missed']}")