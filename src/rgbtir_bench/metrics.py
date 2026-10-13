from __future__ import annotations

import cv2
import numpy as np

from .matchers.base import MatchResult


def evaluate_matches(result: MatchResult, shape0: tuple[int, ...], shape1: tuple[int, ...], correctness_px: float, ransac_px: float) -> dict[str, float | int]:
    n = len(result.points0)
    if n == 0:
        return _empty_metrics()
    h0, w0 = shape0[:2]
    h1, w1 = shape1[:2]
    p0 = result.points0.astype(np.float32)
    p1 = result.points1.astype(np.float32).copy()
    p1[:, 0] *= w0 / w1
    p1[:, 1] *= h0 / h1
    errors = np.linalg.norm(p0 - p1, axis=1)
    correct = errors <= correctness_px
    homography_inliers = 0
    homography_rmse = float("nan")
    if n >= 4:
        H, mask = cv2.findHomography(p0, p1, cv2.RANSAC, ransac_px)
        if H is not None and mask is not None:
            mask = mask.ravel().astype(bool)
            homography_inliers = int(mask.sum())
            projected = cv2.perspectiveTransform(p0[None], H)[0]
            if mask.any():
                homography_rmse = float(np.sqrt(np.mean(np.sum((projected[mask] - p1[mask]) ** 2, axis=1))))
    grid_x = np.clip((p0[:, 0] / max(w0, 1) * 4).astype(int), 0, 3)
    grid_y = np.clip((p0[:, 1] / max(h0, 1) * 4).astype(int), 0, 3)
    coverage = len(set(zip(grid_x.tolist(), grid_y.tolist()))) / 16.0
    return {
        "matches": n,
        "correct_matches": int(correct.sum()),
        "precision_px": float(correct.mean()),
        "mean_error_px": float(errors.mean()),
        "median_error_px": float(np.median(errors)),
        "p95_error_px": float(np.percentile(errors, 95)),
        "mma_1px": float((errors <= 1).mean()),
        "mma_3px": float((errors <= 3).mean()),
        "mma_5px": float((errors <= 5).mean()),
        "mma_10px": float((errors <= 10).mean()),
        "homography_inliers": homography_inliers,
        "inlier_ratio": homography_inliers / n,
        "homography_rmse_px": homography_rmse,
        "coverage_4x4": coverage,
    }


def _empty_metrics() -> dict[str, float | int]:
    return {
        "matches": 0, "correct_matches": 0, "precision_px": 0.0,
        "mean_error_px": float("nan"), "median_error_px": float("nan"), "p95_error_px": float("nan"),
        "mma_1px": 0.0, "mma_3px": 0.0, "mma_5px": 0.0, "mma_10px": 0.0,
        "homography_inliers": 0, "inlier_ratio": 0.0, "homography_rmse_px": float("nan"), "coverage_4x4": 0.0,
    }

