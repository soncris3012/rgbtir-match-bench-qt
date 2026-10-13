from __future__ import annotations

from dataclasses import dataclass
from typing import Protocol
import numpy as np


@dataclass(slots=True)
class MatchResult:
    points0: np.ndarray
    points1: np.ndarray
    confidence: np.ndarray


class Matcher(Protocol):
    name: str

    def match(self, visible: np.ndarray, thermal: np.ndarray) -> MatchResult: ...


def empty_result() -> MatchResult:
    return MatchResult(
        np.empty((0, 2), dtype=np.float32),
        np.empty((0, 2), dtype=np.float32),
        np.empty((0,), dtype=np.float32),
    )

