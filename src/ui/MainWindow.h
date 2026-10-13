#pragma once

#include "core/BenchmarkTypes.h"

#include <QMainWindow>

class QDoubleSpinBox;
class QLabel;
class QLineEdit;
class QListWidget;
class QProgressBar;
class QPushButton;
class QSpinBox;
class QTableWidget;
class QThread;

namespace rgbtir {

class BenchmarkWorker;
class DropZone;

class MainWindow final : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    void chooseDatasetRoot();
    void validateDataset();
    void startBenchmark();
    void stopBenchmark();
    void updateProgress(int current, int total, const QString &message);
    void showCurrentPair(
        const QString &visiblePath,
        const QString &thermalPath,
        const QString &method,
        int current,
        int total);
    void showResult(const rgbtir::BenchmarkResult &result);
    void showFailure(const QString &message);
    void openReport();
    void openRunDirectory();

private:
    void buildUi();
    void applyMonochromeStyle();
    QStringList selectedMethods() const;
    void setRunning(bool running);
    void setDatasetInputs(const QString &visible, const QString &thermal, const QString &name);
    void updatePreview(const QString &visiblePath, const QString &thermalPath);

    QLineEdit *datasetRoot_ = nullptr;
    DropZone *visibleDrop_ = nullptr;
    DropZone *thermalDrop_ = nullptr;
    QLabel *datasetStatus_ = nullptr;
    QLabel *datasetType_ = nullptr;
    QLabel *visiblePreview_ = nullptr;
    QLabel *thermalPreview_ = nullptr;
    QLabel *visibleName_ = nullptr;
    QLabel *thermalName_ = nullptr;
    QListWidget *methodList_ = nullptr;
    QSpinBox *maxPairs_ = nullptr;
    QDoubleSpinBox *correctness_ = nullptr;
    QDoubleSpinBox *ransac_ = nullptr;
    QSpinBox *maxKeypoints_ = nullptr;
    QPushButton *runButton_ = nullptr;
    QPushButton *stopButton_ = nullptr;
    QPushButton *openReportButton_ = nullptr;
    QPushButton *openRunDirectoryButton_ = nullptr;
    QProgressBar *progressBar_ = nullptr;
    QLabel *statusLabel_ = nullptr;
    QTableWidget *resultTable_ = nullptr;
    QThread *workerThread_ = nullptr;
    BenchmarkWorker *worker_ = nullptr;
    QString datasetName_ = QStringLiteral("Auto");
    QString lastRunDirectory_;
    bool updatingInputs_ = false;
};

} // namespace rgbtir

