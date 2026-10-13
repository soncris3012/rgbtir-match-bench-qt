from pathlib import Path

import cv2
import numpy as np

from rgbtir_bench.datasets import detect_dataset_root, pair_directories


def _image(path: Path):
    path.parent.mkdir(parents=True, exist_ok=True)
    cv2.imwrite(str(path), np.zeros((12, 16, 3), np.uint8))


def test_pair_llvip_layout(tmp_path):
    _image(tmp_path / "visible" / "test" / "190001.jpg")
    _image(tmp_path / "infrared" / "test" / "190001.jpg")
    visible, thermal, name = detect_dataset_root(tmp_path)
    pairs, warnings = pair_directories(visible, thermal)
    assert name == "LLVIP"
    assert len(pairs) == 1
    assert not warnings


def test_pair_sorted_fallback(tmp_path):
    _image(tmp_path / "v" / "a.jpg")
    _image(tmp_path / "t" / "x.jpg")
    pairs, warnings = pair_directories(tmp_path / "v", tmp_path / "t")
    assert len(pairs) == 1
    assert warnings

