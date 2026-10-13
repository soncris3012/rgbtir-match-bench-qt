from __future__ import annotations

from dataclasses import asdict, dataclass
from pathlib import Path


@dataclass(slots=True)
class BenchmarkConfig:
    visible_dir: Path
    thermal_dir: Path
    methods: list[str]
    output_dir: Path
    dataset_name: str = "Auto"
    max_pairs: int = 100
    correctness_px: float = 5.0
    ransac_px: float = 3.0
    max_keypoints: int = 2048
    device: str = "auto"
    seed: int = 42

    def to_dict(self) -> dict:
        value = asdict(self)
        value["visible_dir"] = str(self.visible_dir)
        value["thermal_dir"] = str(self.thermal_dir)
        value["output_dir"] = str(self.output_dir)
        return value

