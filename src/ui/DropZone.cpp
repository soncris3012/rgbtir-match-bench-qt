#include "ui/DropZone.h"

#include <QDir>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileInfo>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QMimeData>
#include <QPushButton>
#include <QStyle>
#include <QUrl>
#include <QVBoxLayout>

namespace rgbtir {

DropZone::DropZone(const QString &title, const QString &hint, QWidget *parent)
    : QFrame(parent)
{
    setAcceptDrops(true);
    setObjectName(QStringLiteral("dropZone"));
    setProperty("active", false);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(18, 16, 18, 16);
    layout->setSpacing(7);
    auto *titleLabel = new QLabel(title, this);
    titleLabel->setObjectName(QStringLiteral("dropTitle"));
    valueLabel_ = new QLabel(hint, this);
    valueLabel_->setObjectName(QStringLiteral("dropValue"));
    valueLabel_->setWordWrap(true);
    auto *caption = new QLabel(QStringLiteral("KÉO ẢNH HOẶC THƯ MỤC VÀO ĐÂY"), this);
    caption->setObjectName(QStringLiteral("dropCaption"));
    auto *buttons = new QHBoxLayout;
    buttons->setSpacing(6);
    auto *imageButton = new QPushButton(QStringLiteral("MỞ ẢNH…"), this);
    auto *directoryButton = new QPushButton(QStringLiteral("MỞ THƯ MỤC…"), this);
    imageButton->setObjectName(QStringLiteral("smallButton"));
    directoryButton->setObjectName(QStringLiteral("smallButton"));
    buttons->addWidget(imageButton);
    buttons->addWidget(directoryButton);
    layout->addWidget(titleLabel);
    layout->addWidget(valueLabel_, 1);
    layout->addWidget(caption);
    layout->addLayout(buttons);
    connect(imageButton, &QPushButton::clicked, this, &DropZone::chooseImage);
    connect(directoryButton, &QPushButton::clicked, this, &DropZone::chooseDirectory);
}

void DropZone::setPath(const QString &path)
{
    path_ = path.isEmpty() ? QString() : QDir::cleanPath(path);
    valueLabel_->setText(path_.isEmpty() ? QStringLiteral("Chưa chọn thư mục") : path_);
    setToolTip(path_);
    emit pathChanged(path_);
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
        QStringLiteral("Ảnh (*.jpg *.jpeg *.png *.bmp *.tif *.tiff);;Tất cả tệp (*)"));
    if (!file.isEmpty())
        setPath(file);
}

void DropZone::chooseDirectory()
{
    const QString initial = path_.isEmpty()
        ? QString()
        : (QFileInfo(path_).isDir() ? path_ : QFileInfo(path_).absolutePath());
    const QString directory = QFileDialog::getExistingDirectory(
        this, QStringLiteral("Chọn thư mục ảnh"), initial);
    if (!directory.isEmpty())
        setPath(directory);
}

} // namespace rgbtir

