from __future__ import annotations

from pathlib import Path

from PySide6.QtCore import Signal
from PySide6.QtGui import QDragEnterEvent, QDropEvent
from PySide6.QtWidgets import QFrame, QLabel, QVBoxLayout


class FolderDrop(QFrame):
    folderChanged = Signal(str)

    def __init__(self, title: str, hint: str, parent=None):
        super().__init__(parent)
        self.setAcceptDrops(True)
        self.setObjectName("folderDrop")
        self._path = ""
        layout = QVBoxLayout(self)
        self.title = QLabel(title)
        self.title.setObjectName("dropTitle")
        self.value = QLabel(hint)
        self.value.setWordWrap(True)
        self.value.setObjectName("dropHint")
        layout.addWidget(self.title)
        layout.addWidget(self.value)

    @property
    def path(self) -> str:
        return self._path

    def set_path(self, path: str) -> None:
        self._path = path
        self.value.setText(path or "Kéo thư mục vào đây")
        self.setToolTip(path)
        self.folderChanged.emit(path)

    def dragEnterEvent(self, event: QDragEnterEvent) -> None:
        urls = event.mimeData().urls()
        if urls and Path(urls[0].toLocalFile()).is_dir():
            event.acceptProposedAction()

    def dropEvent(self, event: QDropEvent) -> None:
        path = event.mimeData().urls()[0].toLocalFile()
        if Path(path).is_dir():
            self.set_path(path)
            event.acceptProposedAction()

