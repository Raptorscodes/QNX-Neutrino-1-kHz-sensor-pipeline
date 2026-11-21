#!/usr/bin/env python3
"""
Stage 3 — plot timer jitter from a CSV produced by sensor_jitter.

Expected CSV columns: n,jitter_ns  (header row required)


"""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

import matplotlib.pyplot as plt
import numpy as np


def main() -> int:
    parser = argparse.ArgumentParser(description="Plot QNX timer jitter CSV")
    parser.add_argument("csv_path", type=Path, help="path to jitter CSV")
    parser.add_argument(
        "-o",
        "--output",
        type=Path,
        default=None,
        help="optional output image path (e.g. results/qnx/jitter_idle.png)",
    )
    args = parser.parse_args()

    if not args.csv_path.is_file():
        print(f"file not found: {args.csv_path}", file=sys.stderr)
        return 1

    data = np.loadtxt(args.csv_path, delimiter=",", skiprows=1)
    if data.ndim == 1:
        # Single row edge case
        jitter_ns = np.array([data[1]], dtype=np.float64)
    else:
        jitter_ns = data[:, 1].astype(np.float64)

    if jitter_ns.size == 0:
        print("CSV has no samples", file=sys.stderr)
        return 1

    median = float(np.median(jitter_ns))
    p99 = float(np.percentile(jitter_ns, 99))
    p999 = float(np.percentile(jitter_ns, 99.9))
    jmax = float(np.max(jitter_ns))
    jmin = float(np.min(jitter_ns))
    abs_max = float(np.max(np.abs(jitter_ns)))

    print(f"file:     {args.csv_path}")
    print(f"samples:  {jitter_ns.size}")
    print(f"median:   {median:.0f} ns")
    print(f"p99:      {p99:.0f} ns")
    print(f"p99.9:    {p999:.0f} ns")
    print(f"min:      {jmin:.0f} ns")
    print(f"max:      {jmax:.0f} ns")
    print(f"abs max:  {abs_max:.0f} ns")

    fig, ax = plt.subplots(figsize=(8, 5))
    ax.hist(jitter_ns, bins=100)
    ax.set_xlabel("jitter (ns)")
    ax.set_ylabel("count")
    ax.set_title(f"Timer jitter — {args.csv_path.name}")
    ax.axvline(0.0, linestyle="--", linewidth=1)
    fig.tight_layout()

    if args.output is not None:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        fig.savefig(args.output, dpi=150)
        print(f"wrote {args.output}")
    else:
        plt.show()

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
