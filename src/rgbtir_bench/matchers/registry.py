from __future__ import annotations

from .classical import OpenCVMatcher
from .lightglue_backend import LightGlueMatcher
from .loftr_backend import LoFTRMatcher


METHODS = ["SIFT", "ORB", "SuperPoint + LightGlue", "DISK + LightGlue", "ALIKED + LightGlue", "LoFTR"]


def create_matcher(name: str, max_keypoints: int = 2048, device: str = "auto"):
    if name in {"SIFT", "ORB"}:
        return OpenCVMatcher(name, max_keypoints=max_keypoints)
    if name in LightGlueMatcher.FEATURE_MAP:
        return LightGlueMatcher(name, max_keypoints=max_keypoints, device=device)
    if name == "LoFTR":
        return LoFTRMatcher(device=device)
    raise ValueError(f"Unknown method: {name}")

