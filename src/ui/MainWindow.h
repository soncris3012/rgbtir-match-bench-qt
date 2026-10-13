#pragma once

#include "core/BenchmarkTypes.h"

#include <QMainWindow>

class QDoubleSpinBox;
class QLabel;
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
    void browseVisible();
    void browseThermal();
    void browseRoot();
    void validateDataset();
    void startBenchmark();
    void stopBenchmark();
    void updateProgress(int current, int total, const QString &message);
    void showResult(const rgbtir::BenchmarkResult &result);
    void showFailure(const QString &message);
    void openReport();

private:
    void buildUi();
    void applyMonochromeStyle();
    QStringList selectedMethods() const;
    void setRunning(bool running);

    DropZone *visibleDrop_ = nullptr;
    DropZone *thermalDrop_ = nullptr;
    QLabel *datasetStatus_ = nullptr;
    QListWidget *methodList_ = nullptr;
    QSpinBox *maxPairs_ = nullptr;
    QDoubleSpinBox *correctness_ = nullptr;
    QDoubleSpinBox *ransac_ = nullptr;
    QSpinBox *maxKeypoints_ = nullptr;
    QPushButton *runButton_ = nullptr;
    QPushButton *stopButton_ = nullptr;
    QPushButton *openReportButton_ = nullptr;
    QProgressBar *progressBar_ = nullptr;
    QLabel *statusLabel_ = nullptr;
    QTableWidget *resultTable_ = nullptr;
    QThread *workerThread_ = nullptr;
    BenchmarkWorker *worker_ = nullptr;
    QString datasetName_ = QStringLiteral("Auto");
    QString lastRunDirectory_;
};

} // namespace rgbtir

