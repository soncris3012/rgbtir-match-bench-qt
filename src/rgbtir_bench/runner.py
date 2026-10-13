from __future__ import annotations

from datetime import datetime
import random
import time
from typing import Callable

import cv2

from .config import BenchmarkConfig
from .datasets import pair_directories
from .matchers.registry import create_matcher
from .metrics import evaluate_matches
from .report import write_reports


def run_benchmark(config: BenchmarkConfig, progress: Callable[[int, int, str], None] | None = None, cancelled: Callable[[], bool] | None = None):
    pairs, warnings = pair_directories(config.visible_dir, config.thermal_dir)
    rng = random.Random(config.seed)
    if config.max_pairs and len(pairs) > config.max_pairs:
        pairs = sorted(rng.sample(pairs, config.max_pairs), key=lambda p: p.pair_id)
    run_dir = config.output_dir / datetime.now().strftime("%Y%m%d-%H%M%S")
    total = len(pairs) * len(config.methods)
    rows, done = [], 0
    for method_name in config.methods:
        try:
            matcher = create_matcher(method_name, config.max_keypoints, config.device)
        except Exception as exc:
            for pair in pairs:
                rows.append({"method": method_name, "pair_id": pair.pair_id, "error": str(exc)})
                done += 1
                if progress:
                    progress(done, total, f"{method_name}: {exc}")
            continue
        for pair in pairs:
            if cancelled and cancelled():
                raise InterruptedError("Đã dừng theo yêu cầu.")
            row = {"method": method_name, "pair_id": pair.pair_id, "visible": str(pair.visible), "thermal": str(pair.thermal)}
            try:
                visible = cv2.imread(str(pair.visible), cv2.IMREAD_COLOR)
                thermal = cv2.imread(str(pair.thermal), cv2.IMREAD_COLOR)
                if visible is None or thermal is None:
                    raise ValueError("Không đọc được ảnh")
                start = time.perf_counter()
                result = matcher.match(visible, thermal)
                row.update(evaluate_matches(result, visible.shape, thermal.shape, config.correctness_px, config.ransac_px))
                row["runtime_ms"] = (time.perf_counter() - start) * 1000
                row["error"] = ""
            except Exception as exc:
                row["error"] = f"{type(exc).__name__}: {exc}"
            rows.append(row)
            done += 1
            if progress:
                progress(done, total, f"{method_name} · {pair.pair_id}")
    summary = write_reports(run_dir, config.to_dict(), rows)
    return run_dir, summary, warnings

