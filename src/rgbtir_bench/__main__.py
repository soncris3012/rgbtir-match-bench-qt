from __future__ import annotations

import argparse
from pathlib import Path
import sys


def main() -> int:
    parser = argparse.ArgumentParser(description="RGB–TIR local feature benchmark")
    parser.add_argument("--visible", type=Path)
    parser.add_argument("--thermal", type=Path)
    parser.add_argument("--methods", nargs="+", default=["SIFT"])
    parser.add_argument("--max-pairs", type=int, default=100)
    parser.add_argument("--threshold", type=float, default=5.0)
    parser.add_argument("--output", type=Path, default=Path("runs"))
    parser.add_argument("--device", default="auto")
    args = parser.parse_args()
    if args.visible and args.thermal:
        from .config import BenchmarkConfig
        from .runner import run_benchmark
        config = BenchmarkConfig(args.visible, args.thermal, args.methods, args.output, max_pairs=args.max_pairs, correctness_px=args.threshold, device=args.device)
        run_dir, summary, warnings = run_benchmark(config, lambda n, total, text: print(f"[{n}/{total}] {text}"))
        print(f"Report: {run_dir / 'report.html'}")
        for warning in warnings:
            print(f"Warning: {warning}")
        return 0
    from PySide6.QtWidgets import QApplication
    from .ui.main_window import create_window
    app = QApplication(sys.argv)
    app.setApplicationName("RGB–TIR Match Bench")
    window = create_window()
    window.show()
    return app.exec()


if __name__ == "__main__":
    raise SystemExit(main())

