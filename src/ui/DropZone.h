#pragma once

#include <QFrame>

class QLabel;

namespace rgbtir {

class DropZone final : public QFrame {
    Q_OBJECT
    Q_PROPERTY(QString path READ path WRITE setPath NOTIFY pathChanged)

public:
    explicit DropZone(const QString &title, const QString &hint, QWidget *parent = nullptr);

    QString path() const { return path_; }
    void setPath(const QString &path);

signals:
    void pathChanged(const QString &path);

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragLeaveEvent(QDragLeaveEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private:
    void setActive(bool active);

    QString path_;
    QLabel *valueLabel_ = nullptr;
};

} // namespace rgbtir

