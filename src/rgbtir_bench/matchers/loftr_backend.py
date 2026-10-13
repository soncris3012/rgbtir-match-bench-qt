from __future__ import annotations

import cv2
import numpy as np

from .base import MatchResult, empty_result
from .lightglue_backend import _torch_device


class LoFTRMatcher:
    name = "LoFTR"

    def __init__(self, device: str = "auto", **_):
        try:
            import torch
            from kornia.feature import LoFTR
        except ImportError as exc:
            raise RuntimeError("Thiếu backend LoFTR. Cài: pip install -r requirements-deep.txt") from exc
        self.torch = torch
        self.device = _torch_device(torch, device)
        self.matcher = LoFTR(pretrained="outdoor").eval().to(self.device)

    def _tensor(self, image: np.ndarray):
        gray = cv2.cvtColor(image, cv2.COLOR_BGR2GRAY) if image.ndim == 3 else image
        return self.torch.from_numpy(gray).float()[None, None].div(255).to(self.device)

    def match(self, visible: np.ndarray, thermal: np.ndarray) -> MatchResult:
        with self.torch.inference_mode():
            out = self.matcher({"image0": self._tensor(visible), "image1": self._tensor(thermal)})
        if out["keypoints0"].numel() == 0:
            return empty_result()
        return MatchResult(
            out["keypoints0"].cpu().numpy().astype(np.float32),
            out["keypoints1"].cpu().numpy().astype(np.float32),
            out["confidence"].cpu().numpy().astype(np.float32),
        )

