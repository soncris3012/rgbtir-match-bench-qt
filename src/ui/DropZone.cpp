#include "ui/DropZone.h"

#include <QDir>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileInfo>
#include <QLabel>
#include <QMimeData>
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
    auto *caption = new QLabel(QStringLiteral("THẢ THƯ MỤC VÀO KHUNG NÀY"), this);
    caption->setObjectName(QStringLiteral("dropCaption"));
    layout->addWidget(titleLabel);
    layout->addWidget(valueLabel_, 1);
    layout->addWidget(caption);
}

void DropZone::setPath(const QString &path)
{
    path_ = QDir::cleanPath(path);
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
    if (!urls.isEmpty() && QFileInfo(urls.first().toLocalFile()).isDir()) {
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
    if (!urls.isEmpty() && QFileInfo(urls.first().toLocalFile()).isDir()) {
        setPath(urls.first().toLocalFile());
        event->acceptProposedAction();
    }
}

} // namespace rgbtir

