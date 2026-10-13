#!/usr/bin/env python3
from __future__ import annotations

import argparse
from pathlib import Path

from rgbtir_bench.datasets import detect_dataset_root, pair_directories


def main() -> int:
    parser = argparse.ArgumentParser(description="Validate LLVIP/RoadScene folder layout and pairing")
    parser.add_argument("root", type=Path)
    args = parser.parse_args()
    visible, thermal, name = detect_dataset_root(args.root)
    pairs, warnings = pair_directories(visible, thermal)
    print(f"Dataset: {name}")
    print(f"Visible: {visible}")
    print(f"Thermal: {thermal}")
    print(f"Pairs: {len(pairs)}")
    for warning in warnings:
        print(f"Warning: {warning}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

