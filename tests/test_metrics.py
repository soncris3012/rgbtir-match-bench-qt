import numpy as np

from rgbtir_bench.matchers.base import MatchResult
from rgbtir_bench.metrics import evaluate_matches


def test_identity_matches_are_correct():
    points = np.float32([[10, 10], [80, 10], [80, 80], [10, 80], [50, 50]])
    result = MatchResult(points, points.copy(), np.ones(5, np.float32))
    metrics = evaluate_matches(result, (100, 100, 3), (100, 100, 3), 1.0, 3.0)
    assert metrics["precision_px"] == 1.0
    assert metrics["mma_1px"] == 1.0
    assert metrics["homography_inliers"] == 5
    assert metrics["homography_rmse_px"] < 1e-4


def test_resolution_normalization():
    p0 = np.float32([[10, 10], [50, 10], [50, 50], [10, 50]])
    p1 = p0 * 2
    metrics = evaluate_matches(MatchResult(p0, p1, np.ones(4)), (100, 100, 3), (200, 200, 3), 0.1, 1.0)
    assert metrics["precision_px"] == 1.0

