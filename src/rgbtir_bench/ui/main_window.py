from __future__ import annotations

from pathlib import Path

from PySide6.QtCore import Qt, QThread, QUrl, Signal
from PySide6.QtGui import QDesktopServices
from PySide6.QtWidgets import (
    QAbstractItemView, QApplication, QComboBox, QDoubleSpinBox, QFileDialog,
    QFormLayout, QGridLayout, QGroupBox, QHBoxLayout, QHeaderView, QLabel,
    QListWidget, QListWidgetItem, QMainWindow, QMessageBox, QProgressBar,
    QPushButton, QSpinBox, QSplitter, QTableWidget, QTableWidgetItem,
    QVBoxLayout, QWidget,
)

from ..config import BenchmarkConfig
from ..datasets import detect_dataset_root, pair_directories
from ..matchers.registry import METHODS
from ..runner import run_benchmark
from .widgets import FolderDrop


class BenchmarkThread(QThread):
    progress = Signal(int, int, str)
    completed = Signal(str, object, object)
    failed = Signal(str)

    def __init__(self, config: BenchmarkConfig):
        super().__init__()
        self.config = config
        self._cancelled = False

    def cancel(self):
        self._cancelled = True

    def run(self):
        try:
            run_dir, summary, warnings = run_benchmark(
                self.config,
                lambda n, total, text: self.progress.emit(n, total, text),
                lambda: self._cancelled,
            )
            self.completed.emit(str(run_dir), summary, warnings)
        except Exception as exc:
            self.failed.emit(f"{type(exc).__name__}: {exc}")


class MainWindow(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("RGB–TIR Match Bench")
        self.resize(1180, 760)
        self.worker: BenchmarkThread | None = None
        self.last_run_dir: Path | None = None
        self._build_ui()
        self._apply_style()

    def _build_ui(self):
        root = QWidget()
        self.setCentralWidget(root)
        outer = QVBoxLayout(root)
        heading = QLabel("RGB–TIR Match Bench")
        heading.setObjectName("heading")
        subtitle = QLabel("Kiểm chứng bộ ghép đặc trưng đa phổ trên LLVIP và RoadScene · kéo thả, chạy, xuất báo cáo")
        subtitle.setObjectName("subtitle")
        outer.addWidget(heading)
        outer.addWidget(subtitle)

        splitter = QSplitter()
        outer.addWidget(splitter, 1)
        left, right = QWidget(), QWidget()
        splitter.addWidget(left)
        splitter.addWidget(right)
        splitter.setSizes([460, 700])
        left_layout, right_layout = QVBoxLayout(left), QVBoxLayout(right)

        data_group = QGroupBox("1 · Dữ liệu")
        data_layout = QGridLayout(data_group)
        self.visible_drop = FolderDrop("Ảnh RGB / visible", "Kéo thư mục visible vào đây")
        self.thermal_drop = FolderDrop("Ảnh nhiệt / infrared", "Kéo thư mục infrared vào đây")
        data_layout.addWidget(self.visible_drop, 0, 0)
        data_layout.addWidget(self.thermal_drop, 0, 1)
        browse_v = QPushButton("Chọn RGB…")
        browse_t = QPushButton("Chọn IR…")
        root_button = QPushButton("Nhận dạng thư mục gốc LLVIP / RoadScene…")
        browse_v.clicked.connect(lambda: self._browse(self.visible_drop))
        browse_t.clicked.connect(lambda: self._browse(self.thermal_drop))
        root_button.clicked.connect(self._browse_root)
        data_layout.addWidget(browse_v, 1, 0)
        data_layout.addWidget(browse_t, 1, 1)
        data_layout.addWidget(root_button, 2, 0, 1, 2)
        self.dataset_label = QLabel("Chưa chọn dữ liệu")
        data_layout.addWidget(self.dataset_label, 3, 0, 1, 2)
        self.visible_drop.folderChanged.connect(self._validate_data)
        self.thermal_drop.folderChanged.connect(self._validate_data)
        left_layout.addWidget(data_group)

        method_group = QGroupBox("2 · Phương pháp")
        method_layout = QVBoxLayout(method_group)
        self.methods = QListWidget()
        self.methods.setSelectionMode(QAbstractItemView.NoSelection)
        for index, name in enumerate(METHODS):
            item = QListWidgetItem(name)
            item.setFlags(item.flags() | Qt.ItemIsUserCheckable)
            item.setCheckState(Qt.Checked if index in (0, 2) else Qt.Unchecked)
            self.methods.addItem(item)
        method_layout.addWidget(self.methods)
        note = QLabel("Các phương pháp DL tự tải trọng số ở lần chạy đầu. SIFT/ORB chạy không cần GPU.")
        note.setWordWrap(True)
        method_layout.addWidget(note)
        left_layout.addWidget(method_group, 1)

        settings = QGroupBox("3 · Thiết lập")
        form = QFormLayout(settings)
        self.max_pairs = QSpinBox(); self.max_pairs.setRange(1, 100000); self.max_pairs.setValue(100)
        self.threshold = QDoubleSpinBox(); self.threshold.setRange(0.1, 50); self.threshold.setValue(5); self.threshold.setSuffix(" px")
        self.ransac = QDoubleSpinBox(); self.ransac.setRange(0.1, 50); self.ransac.setValue(3); self.ransac.setSuffix(" px")
        self.keypoints = QSpinBox(); self.keypoints.setRange(128, 16384); self.keypoints.setValue(2048); self.keypoints.setSingleStep(128)
        self.device = QComboBox(); self.device.addItems(["auto", "cpu", "mps", "cuda"])
        form.addRow("Số cặp tối đa", self.max_pairs)
        form.addRow("Ngưỡng đúng", self.threshold)
        form.addRow("RANSAC", self.ransac)
        form.addRow("Keypoint tối đa", self.keypoints)
        form.addRow("Thiết bị", self.device)
        left_layout.addWidget(settings)

        controls = QHBoxLayout()
        self.run_button = QPushButton("Chạy benchmark")
        self.run_button.setObjectName("primary")
        self.stop_button = QPushButton("Dừng")
        self.stop_button.setEnabled(False)
        self.run_button.clicked.connect(self._run)
        self.stop_button.clicked.connect(self._stop)
        controls.addWidget(self.run_button, 1)
        controls.addWidget(self.stop_button)
        left_layout.addLayout(controls)

        result_group = QGroupBox("Kết quả tổng hợp")
        result_layout = QVBoxLayout(result_group)
        self.table = QTableWidget(0, 8)
        self.table.setHorizontalHeaderLabels(["Method", "Pairs", "Precision", "MMA@3", "Inliers", "Median px", "Coverage", "ms/pair"])
        self.table.horizontalHeader().setSectionResizeMode(QHeaderView.Stretch)
        self.table.setAlternatingRowColors(True)
        result_layout.addWidget(self.table)
        right_layout.addWidget(result_group, 1)

        self.progress = QProgressBar()
        self.status = QLabel("Sẵn sàng")
        self.status.setWordWrap(True)
        self.open_report = QPushButton("Mở báo cáo HTML")
        self.open_report.setEnabled(False)
        self.open_report.clicked.connect(self._open_report)
        right_layout.addWidget(self.progress)
        right_layout.addWidget(self.status)
        right_layout.addWidget(self.open_report)

    def _browse(self, target: FolderDrop):
        path = QFileDialog.getExistingDirectory(self, "Chọn thư mục ảnh")
        if path:
            target.set_path(path)

    def _browse_root(self):
        path = QFileDialog.getExistingDirectory(self, "Chọn thư mục gốc LLVIP hoặc RoadScene")
        if not path:
            return
        try:
            visible, thermal, name = detect_dataset_root(Path(path))
            self.visible_drop.set_path(str(visible))
            self.thermal_drop.set_path(str(thermal))
            self.dataset_label.setText(f"Đã nhận dạng: {name}")
        except ValueError as exc:
            QMessageBox.warning(self, "Không nhận dạng được", str(exc))

    def _validate_data(self):
        if not self.visible_drop.path or not self.thermal_drop.path:
            return
        try:
            pairs, warnings = pair_directories(Path(self.visible_drop.path), Path(self.thermal_drop.path))
            extra = f" · {warnings[0]}" if warnings else ""
            self.dataset_label.setText(f"✓ {len(pairs)} cặp hợp lệ{extra}")
        except ValueError as exc:
            self.dataset_label.setText(f"⚠ {exc}")

    def _selected_methods(self) -> list[str]:
        return [self.methods.item(i).text() for i in range(self.methods.count()) if self.methods.item(i).checkState() == Qt.Checked]

    def _run(self):
        methods = self._selected_methods()
        if not methods:
            QMessageBox.warning(self, "Thiếu phương pháp", "Chọn ít nhất một phương pháp.")
            return
        try:
            pairs, _ = pair_directories(Path(self.visible_drop.path), Path(self.thermal_drop.path))
        except ValueError as exc:
            QMessageBox.warning(self, "Dữ liệu chưa hợp lệ", str(exc))
            return
        output = Path.cwd() / "runs"
        config = BenchmarkConfig(
            visible_dir=Path(self.visible_drop.path), thermal_dir=Path(self.thermal_drop.path),
            methods=methods, output_dir=output, dataset_name="Auto",
            max_pairs=min(self.max_pairs.value(), len(pairs)), correctness_px=self.threshold.value(),
            ransac_px=self.ransac.value(), max_keypoints=self.keypoints.value(), device=self.device.currentText(),
        )
        self.table.setRowCount(0)
        self.progress.setValue(0)
        self.run_button.setEnabled(False)
        self.stop_button.setEnabled(True)
        self.open_report.setEnabled(False)
        self.worker = BenchmarkThread(config)
        self.worker.progress.connect(self._on_progress)
        self.worker.completed.connect(self._on_completed)
        self.worker.failed.connect(self._on_failed)
        self.worker.start()

    def _stop(self):
        if self.worker:
            self.worker.cancel()
            self.status.setText("Đang dừng sau cặp hiện tại…")

    def _on_progress(self, current: int, total: int, text: str):
        self.progress.setMaximum(total)
        self.progress.setValue(current)
        self.status.setText(f"{current}/{total} · {text}")

    def _on_completed(self, run_dir: str, summary: list[dict], warnings: list[str]):
        self.last_run_dir = Path(run_dir)
        self.table.setRowCount(len(summary))
        for row_index, row in enumerate(summary):
            values = [row["method"], row["pairs"], _pct(row.get("precision_px")), _pct(row.get("mma_3px")), _pct(row.get("inlier_ratio")), _num(row.get("median_error_px")), _pct(row.get("coverage_4x4")), _num(row.get("runtime_ms"))]
            for col, value in enumerate(values):
                self.table.setItem(row_index, col, QTableWidgetItem(str(value)))
        self.status.setText(f"Hoàn tất · {run_dir}" + (" · " + " ".join(warnings) if warnings else ""))
        self.run_button.setEnabled(True)
        self.stop_button.setEnabled(False)
        self.open_report.setEnabled(True)

    def _on_failed(self, error: str):
        self.status.setText(error)
        self.run_button.setEnabled(True)
        self.stop_button.setEnabled(False)
        QMessageBox.critical(self, "Benchmark thất bại", error)

    def _open_report(self):
        if self.last_run_dir:
            QDesktopServices.openUrl(QUrl.fromLocalFile(str(self.last_run_dir / "report.html")))

    def _apply_style(self):
        self.setStyleSheet("""
            QMainWindow { background:#f6f7fb; } QWidget { font-size:13px; color:#172033; }
            #heading { font-size:28px; font-weight:700; color:#312e81; } #subtitle { color:#64748b; margin-bottom:8px; }
            QGroupBox { font-weight:600; border:1px solid #dfe3ee; border-radius:12px; margin-top:12px; padding:12px; background:white; }
            QGroupBox::title { subcontrol-origin:margin; left:12px; padding:0 5px; }
            #folderDrop { border:2px dashed #a5b4fc; border-radius:10px; min-height:92px; background:#f5f7ff; }
            #dropTitle { font-weight:700; color:#4338ca; } #dropHint { color:#64748b; }
            QPushButton { border:1px solid #cbd5e1; padding:8px 12px; border-radius:8px; background:white; }
            QPushButton:hover { background:#eef2ff; } QPushButton:disabled { color:#94a3b8; }
            #primary { background:#4f46e5; color:white; border:none; font-weight:700; padding:11px; }
            QTableWidget { border:0; gridline-color:#e2e8f0; background:white; }
            QHeaderView::section { background:#eef2ff; padding:8px; border:0; font-weight:700; }
            QProgressBar { border:0; background:#e2e8f0; border-radius:6px; height:12px; text-align:center; }
            QProgressBar::chunk { background:#4f46e5; border-radius:6px; }
        """)


def _pct(value):
    return "—" if value is None else f"{100 * value:.2f}%"


def _num(value):
    return "—" if value is None else f"{value:.2f}"


def create_window() -> MainWindow:
    return MainWindow()

