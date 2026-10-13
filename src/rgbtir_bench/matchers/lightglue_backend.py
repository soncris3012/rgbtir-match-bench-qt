from __future__ import annotations

import cv2
import numpy as np

from .base import MatchResult, empty_result


class LightGlueMatcher:
    FEATURE_MAP = {
        "SuperPoint + LightGlue": ("SuperPoint", "superpoint"),
        "DISK + LightGlue": ("DISK", "disk"),
        "ALIKED + LightGlue": ("ALIKED", "aliked"),
    }

    def __init__(self, name: str, max_keypoints: int = 2048, device: str = "auto"):
        try:
            import torch
            import lightglue
            from lightglue import LightGlue
        except ImportError as exc:
            raise RuntimeError("Thiếu backend LightGlue. Cài: pip install -r requirements-deep.txt") from exc
        if name not in self.FEATURE_MAP:
            raise ValueError(name)
        extractor_name, feature_name = self.FEATURE_MAP[name]
        extractor_cls = getattr(lightglue, extractor_name)
        self.torch = torch
        self.name = name
        self.device = _torch_device(torch, device)
        self.extractor = extractor_cls(max_num_keypoints=max_keypoints).eval().to(self.device)
        self.matcher = LightGlue(features=feature_name).eval().to(self.device)

    def _tensor(self, image: np.ndarray):
        if image.ndim == 2:
            image = cv2.cvtColor(image, cv2.COLOR_GRAY2RGB)
        else:
            image = cv2.cvtColor(image, cv2.COLOR_BGR2RGB)
        return self.torch.from_numpy(image).permute(2, 0, 1).float().div(255).to(self.device)

    def match(self, visible: np.ndarray, thermal: np.ndarray) -> MatchResult:
        from lightglue.utils import rbd

        with self.torch.inference_mode():
            feats0 = self.extractor.extract(self._tensor(visible))
            feats1 = self.extractor.extract(self._tensor(thermal))
            result = self.matcher({"image0": feats0, "image1": feats1})
            feats0, feats1, result = [rbd(x) for x in (feats0, feats1, result)]
            matches = result["matches"]
            if matches.numel() == 0:
                return empty_result()
            p0 = feats0["keypoints"][matches[:, 0]].detach().cpu().numpy().astype(np.float32)
            p1 = feats1["keypoints"][matches[:, 1]].detach().cpu().numpy().astype(np.float32)
            scores = result.get("scores")
            confidence = scores.detach().cpu().numpy().astype(np.float32) if scores is not None else np.ones(len(p0), np.float32)
            return MatchResult(p0, p1, confidence)


def _torch_device(torch, requested: str) -> str:
    if requested != "auto":
        return requested
    if torch.cuda.is_available():
        return "cuda"
    if getattr(torch.backends, "mps", None) and torch.backends.mps.is_available():
        return "mps"
    return "cpu"

