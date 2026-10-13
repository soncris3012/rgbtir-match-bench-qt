from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
import re


IMAGE_SUFFIXES = {".jpg", ".jpeg", ".png", ".bmp", ".tif", ".tiff"}


@dataclass(frozen=True, slots=True)
class ImagePair:
    pair_id: str
    visible: Path
    thermal: Path


def _images(folder: Path) -> list[Path]:
    if not folder.is_dir():
        return []
    return sorted(p for p in folder.rglob("*") if p.is_file() and p.suffix.lower() in IMAGE_SUFFIXES)


def _key(path: Path) -> str:
    stem = path.stem.lower()
    stem = re.sub(r"(?:^|[_\-.])(infrared|thermal|ir|visible|visual|vis|vi)(?:$|[_\-.])", "_", stem)
    return re.sub(r"[^a-z0-9]+", "", stem)


def pair_directories(visible_dir: Path, thermal_dir: Path) -> tuple[list[ImagePair], list[str]]:
    visible, thermal = _images(visible_dir), _images(thermal_dir)
    if not visible:
        raise ValueError(f"Không tìm thấy ảnh RGB trong: {visible_dir}")
    if not thermal:
        raise ValueError(f"Không tìm thấy ảnh nhiệt trong: {thermal_dir}")

    vmap = {_key(p): p for p in visible}
    tmap = {_key(p): p for p in thermal}
    common = sorted(set(vmap) & set(tmap))
    warnings: list[str] = []
    if common:
        pairs = [ImagePair(k, vmap[k], tmap[k]) for k in common]
    elif len(visible) == len(thermal):
        warnings.append("Tên tệp không khớp; các ảnh được ghép theo thứ tự sắp xếp.")
        pairs = [ImagePair(f"pair-{i:05d}", v, t) for i, (v, t) in enumerate(zip(visible, thermal), 1)]
    else:
        raise ValueError(
            f"Không thể ghép cặp: {len(visible)} ảnh RGB và {len(thermal)} ảnh nhiệt; "
            "hãy dùng cùng tên tệp hoặc cùng số lượng ảnh."
        )
    if len(pairs) < min(len(visible), len(thermal)):
        warnings.append(f"Chỉ ghép được {len(pairs)}/{min(len(visible), len(thermal))} cặp theo tên.")
    return pairs, warnings


def detect_dataset_root(root: Path) -> tuple[Path, Path, str]:
    """Recognize official LLVIP/RoadScene layouts from a dropped root folder."""
    candidates = [
        (root / "visible", root / "infrared", "LLVIP"),
        (root / "LLVIP" / "visible", root / "LLVIP" / "infrared", "LLVIP"),
        (root / "crop_LR_visible", root / "cropinfrared", "RoadScene"),
        (root / "crop_HR_visible", root / "infrared", "RoadScene"),
    ]
    for visible, thermal, name in candidates:
        if visible.is_dir() and thermal.is_dir():
            return visible, thermal, name
    raise ValueError("Không nhận ra cấu trúc LLVIP/RoadScene. Hãy chọn riêng hai thư mục RGB và IR.")

