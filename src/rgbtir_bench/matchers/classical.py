from __future__ import annotations

import cv2
import numpy as np

from .base import MatchResult, empty_result


class OpenCVMatcher:
    def __init__(self, method: str, max_keypoints: int = 2048, ratio: float = 0.8):
        self.name = method.upper()
        self.ratio = ratio
        if self.name == "SIFT":
            self.detector = cv2.SIFT_create(nfeatures=max_keypoints)
            self.norm = cv2.NORM_L2
        elif self.name == "ORB":
            self.detector = cv2.ORB_create(nfeatures=max_keypoints)
            self.norm = cv2.NORM_HAMMING
        else:
            raise ValueError(f"Unsupported OpenCV matcher: {method}")

    def match(self, visible: np.ndarray, thermal: np.ndarray) -> MatchResult:
        gray0 = cv2.cvtColor(visible, cv2.COLOR_BGR2GRAY) if visible.ndim == 3 else visible
        gray1 = cv2.cvtColor(thermal, cv2.COLOR_BGR2GRAY) if thermal.ndim == 3 else thermal
        kp0, des0 = self.detector.detectAndCompute(gray0, None)
        kp1, des1 = self.detector.detectAndCompute(gray1, None)
        if des0 is None or des1 is None or len(des0) < 2 or len(des1) < 2:
            return empty_result()
        knn = cv2.BFMatcher(self.norm).knnMatch(des0, des1, k=2)
        good = [m for m, n in knn if m.distance < self.ratio * n.distance]
        if not good:
            return empty_result()
        p0 = np.float32([kp0[m.queryIdx].pt for m in good])
        p1 = np.float32([kp1[m.trainIdx].pt for m in good])
        distance = np.float32([m.distance for m in good])
        confidence = 1.0 - distance / max(float(distance.max()), 1e-6)
        return MatchResult(p0, p1, confidence)

