#include "ui/DropZone.h"

#include <QDir>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileInfo>
#include <QFileDialog>
#include <QLabel>
#include <QMimeData>
#include <QStyle>
#include <QUrl>

namespace rgbtir {

DropZone::DropZone(QWidget *parent)
    : QFrame(parent)
{
    setAcceptDrops(true);
    setProperty("role", QStringLiteral("dropZone"));
    setProperty("active", false);
}

void DropZone::setPath(const QString &path)
{
    path_ = path.isEmpty() ? QString() : QDir::cleanPath(path);
    if (QLabel *label = valueLabel())
        label->setText(path_.isEmpty() ? QStringLiteral("Chưa chọn ảnh hoặc thư mục") : path_);
    setToolTip(path_);
    emit pathChanged(path_);
}

QLabel *DropZone::valueLabel() const
{
    return findChild<QLabel *>(QStringLiteral("dropValue"));
}

void DropZone::setActive(bool active)
{
    setProperty("active", active);
    style()->unpolish(this);
    style()->polish(this);
}

void DropZone::dragEnterEvent(QDragEnterEvent *event)
{
    const QList<QUrl> urls = event->mimeData()->urls();
    if (!urls.isEmpty() && QFileInfo::exists(urls.first().toLocalFile())) {
        setActive(true);
        event->acceptProposedAction();
    }
}

void DropZone::dragLeaveEvent(QDragLeaveEvent *event)
{
    setActive(false);
    QFrame::dragLeaveEvent(event);
}

void DropZone::dropEvent(QDropEvent *event)
{
    setActive(false);
    const QList<QUrl> urls = event->mimeData()->urls();
    if (!urls.isEmpty() && QFileInfo::exists(urls.first().toLocalFile())) {
        setPath(urls.first().toLocalFile());
        event->acceptProposedAction();
    }
}

void DropZone::chooseImage()
{
    const QString file = QFileDialog::getOpenFileName(
        this,
        QStringLiteral("Chọn ảnh RGB / hồng ngoại"),
        path_.isEmpty() ? QString() : QFileInfo(path_).absolutePath(),
        QStringLiteral("Ảnh (*.jpg *.jpeg *.png *.bmp *.tif *.tiff);;Tất cả tệp (*)"),
        nullptr,
        QFileDialog::DontUseNativeDialog);
    if (!file.isEmpty())
        setPath(file);
}

void DropZone::chooseDirectory()
{
    const QString initial = path_.isEmpty()
        ? QString()
        : (QFileInfo(path_).isDir() ? path_ : QFileInfo(path_).absolutePath());
    const QString directory = QFileDialog::getExistingDirectory(
        this,
        QStringLiteral("Chọn thư mục ảnh"),
        initial,
        QFileDialog::ShowDirsOnly | QFileDialog::DontUseNativeDialog);
    if (!directory.isEmpty())
        setPath(directory);
}

} // namespace rgbtir

