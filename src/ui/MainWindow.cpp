#include "ui/MainWindow.h"

#include "core/DatasetPairer.h"
#include "core/FeatureMatcher.h"
#include "ui/DropZone.h"
#include "worker/BenchmarkWorker.h"

#include <QDesktopServices>
#include <QDir>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QListWidget>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QSpinBox>
#include <QSplitter>
#include <QStandardPaths>
#include <QTableWidget>
#include <QThread>
#include <QUrl>
#include <QVBoxLayout>

namespace rgbtir {
namespace {

QString percent(double value)
{
    return QStringLiteral("%1%").arg(value * 100.0, 0, 'f', 2);
}

QString number(double value)
{
    return QString::number(value, 'f', 2);
}

QLabel *sectionLabel(const QString &numberText, const QString &title, QWidget *parent)
{
    auto *label = new QLabel(QStringLiteral("%1  %2").arg(numberText, title), parent);
    label->setObjectName(QStringLiteral("sectionLabel"));
    return label;
}

} // namespace

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    buildUi();
    applyMonochromeStyle();
}

MainWindow::~MainWindow()
{
    if (worker_)
        worker_->cancel();
    if (workerThread_) {
        workerThread_->quit();
        workerThread_->wait();
    }
}

void MainWindow::buildUi()
{
    setWindowTitle(QStringLiteral("RGB–TIR Match Bench · C++"));
    resize(1240, 800);
    setMinimumSize(980, 680);

    auto *central = new QWidget(this);
    setCentralWidget(central);
    auto *outer = new QVBoxLayout(central);
    outer->setContentsMargins(24, 20, 24, 20);
    outer->setSpacing(8);

    auto *title = new QLabel(QStringLiteral("RGB–TIR MATCH BENCH"), central);
    title->setObjectName(QStringLiteral("appTitle"));
    auto *subtitle = new QLabel(
        QStringLiteral("C++17 · Qt 6 · OpenCV  |  Đánh giá ghép đặc trưng trên LLVIP và RoadScene"), central);
    subtitle->setObjectName(QStringLiteral("subtitle"));
    outer->addWidget(title);
    outer->addWidget(subtitle);

    auto *rule = new QFrame(central);
    rule->setFrameShape(QFrame::HLine);
    rule->setObjectName(QStringLiteral("rule"));
    outer->addWidget(rule);

    auto *splitter = new QSplitter(Qt::Horizontal, central);
    splitter->setChildrenCollapsible(false);
    outer->addWidget(splitter, 1);
    auto *left = new QWidget(splitter);
    auto *right = new QWidget(splitter);
    splitter->addWidget(left);
    splitter->addWidget(right);
    splitter->setSizes({470, 730});

    auto *leftLayout = new QVBoxLayout(left);
    leftLayout->setContentsMargins(0, 8, 12, 0);
    leftLayout->setSpacing(10);
    leftLayout->addWidget(sectionLabel(QStringLiteral("01"), QStringLiteral("DỮ LIỆU"), left));

    auto *dropLayout = new QHBoxLayout;
    dropLayout->setSpacing(10);
    visibleDrop_ = new DropZone(QStringLiteral("RGB / VISIBLE"), QStringLiteral("Thả thư mục ảnh RGB"), left);
    thermalDrop_ = new DropZone(QStringLiteral("THERMAL / IR"), QStringLiteral("Thả thư mục ảnh nhiệt"), left);
    dropLayout->addWidget(visibleDrop_);
    dropLayout->addWidget(thermalDrop_);
    leftLayout->addLayout(dropLayout);

    auto *rootButton = new QPushButton(QStringLiteral("CHỌN THƯ MỤC GỐC LLVIP / ROADSCENE"), left);
    rootButton->setObjectName(QStringLiteral("outlineButton"));
    leftLayout->addWidget(rootButton);
    datasetStatus_ = new QLabel(QStringLiteral("Chưa chọn dữ liệu"), left);
    datasetStatus_->setObjectName(QStringLiteral("datasetStatus"));
    datasetStatus_->setWordWrap(true);
    leftLayout->addWidget(datasetStatus_);

    leftLayout->addWidget(sectionLabel(QStringLiteral("02"), QStringLiteral("PHƯƠNG PHÁP"), left));
    methodList_ = new QListWidget(left);
    methodList_->setObjectName(QStringLiteral("methodList"));
    methodList_->setSelectionMode(QAbstractItemView::NoSelection);
    const QStringList methods = FeatureMatcherFactory::availableMethods();
    for (int i = 0; i < methods.size(); ++i) {
        auto *item = new QListWidgetItem(methods.at(i), methodList_);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(i == 0 ? Qt::Checked : Qt::Unchecked);
    }
    leftLayout->addWidget(methodList_, 1);

    auto *matcherNote = new QLabel(
        QStringLiteral("Baseline C++ thuần OpenCV. SIFT là baseline chính của bài báo; ORB, AKAZE và BRISK dùng để đối chứng."), left);
    matcherNote->setObjectName(QStringLiteral("note"));
    matcherNote->setWordWrap(true);
    leftLayout->addWidget(matcherNote);

    leftLayout->addWidget(sectionLabel(QStringLiteral("03"), QStringLiteral("THIẾT LẬP"), left));
    auto *settings = new QWidget(left);
    auto *form = new QFormLayout(settings);
    form->setContentsMargins(0, 0, 0, 0);
    form->setHorizontalSpacing(20);
    maxPairs_ = new QSpinBox(settings);
    maxPairs_->setRange(1, 100000);
    maxPairs_->setValue(100);
    correctness_ = new QDoubleSpinBox(settings);
    correctness_->setRange(0.1, 50.0);
    correctness_->setValue(5.0);
    correctness_->setSuffix(QStringLiteral(" px"));
    ransac_ = new QDoubleSpinBox(settings);
    ransac_->setRange(0.1, 50.0);
    ransac_->setValue(3.0);
    ransac_->setSuffix(QStringLiteral(" px"));
    maxKeypoints_ = new QSpinBox(settings);
    maxKeypoints_->setRange(128, 16384);
    maxKeypoints_->setSingleStep(128);
    maxKeypoints_->setValue(2048);
    form->addRow(QStringLiteral("Số cặp tối đa"), maxPairs_);
    form->addRow(QStringLiteral("Ngưỡng đúng"), correctness_);
    form->addRow(QStringLiteral("RANSAC"), ransac_);
    form->addRow(QStringLiteral("Keypoint tối đa"), maxKeypoints_);
    leftLayout->addWidget(settings);

    auto *buttonRow = new QHBoxLayout;
    runButton_ = new QPushButton(QStringLiteral("CHẠY BENCHMARK"), left);
    runButton_->setObjectName(QStringLiteral("primaryButton"));
    stopButton_ = new QPushButton(QStringLiteral("DỪNG"), left);
    stopButton_->setObjectName(QStringLiteral("secondaryButton"));
    stopButton_->setEnabled(false);
    buttonRow->addWidget(runButton_, 1);
    buttonRow->addWidget(stopButton_);
    leftLayout->addLayout(buttonRow);

    auto *rightLayout = new QVBoxLayout(right);
    rightLayout->setContentsMargins(12, 8, 0, 0);
    rightLayout->setSpacing(10);
    rightLayout->addWidget(sectionLabel(QStringLiteral("04"), QStringLiteral("KẾT QUẢ TỔNG HỢP"), right));
    resultTable_ = new QTableWidget(0, 8, right);
    resultTable_->setHorizontalHeaderLabels({
        QStringLiteral("METHOD"), QStringLiteral("PAIRS"), QStringLiteral("PRECISION"),
        QStringLiteral("MMA@3"), QStringLiteral("INLIERS"), QStringLiteral("MEDIAN PX"),
        QStringLiteral("COVERAGE"), QStringLiteral("MS/PAIR")});
    resultTable_->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    resultTable_->verticalHeader()->setVisible(false);
    resultTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    resultTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    resultTable_->setAlternatingRowColors(true);
    rightLayout->addWidget(resultTable_, 1);

    progressBar_ = new QProgressBar(right);
    progressBar_->setRange(0, 1);
    progressBar_->setValue(0);
    progressBar_->setTextVisible(false);
    statusLabel_ = new QLabel(QStringLiteral("Sẵn sàng"), right);
    statusLabel_->setObjectName(QStringLiteral("statusLabel"));
    statusLabel_->setWordWrap(true);
    openReportButton_ = new QPushButton(QStringLiteral("MỞ BÁO CÁO HTML"), right);
    openReportButton_->setObjectName(QStringLiteral("outlineButton"));
    openReportButton_->setEnabled(false);
    rightLayout->addWidget(progressBar_);
    rightLayout->addWidget(statusLabel_);
    rightLayout->addWidget(openReportButton_);

    connect(visibleDrop_, &DropZone::browseRequested, this, &MainWindow::browseVisible);
    connect(thermalDrop_, &DropZone::browseRequested, this, &MainWindow::browseThermal);
    connect(visibleDrop_, &DropZone::pathChanged, this, &MainWindow::validateDataset);
    connect(thermalDrop_, &DropZone::pathChanged, this, &MainWindow::validateDataset);
    connect(rootButton, &QPushButton::clicked, this, &MainWindow::browseRoot);
    connect(runButton_, &QPushButton::clicked, this, &MainWindow::startBenchmark);
    connect(stopButton_, &QPushButton::clicked, this, &MainWindow::stopBenchmark);
    connect(openReportButton_, &QPushButton::clicked, this, &MainWindow::openReport);
}

void MainWindow::applyMonochromeStyle()
{
    setStyleSheet(QStringLiteral(R"(
        QMainWindow, QWidget { background:#f5f5f3; color:#111111; font-size:13px; }
        #appTitle { font-size:27px; font-weight:800; letter-spacing:1px; }
        #subtitle, #note, #datasetStatus, #statusLabel { color:#606060; }
        #rule { color:#111111; background:#111111; max-height:1px; }
        #sectionLabel { font-size:12px; font-weight:800; letter-spacing:1.5px; padding:7px 0 2px 0; }
        #dropZone { background:#ffffff; border:2px dashed #858585; border-radius:2px; min-height:118px; }
        #dropZone:hover, #dropZone[active="true"] { border:2px solid #111111; background:#eeeeeb; }
        #dropTitle { font-weight:800; font-size:13px; }
        #dropValue { color:#555555; }
        #dropCaption { color:#777777; font-size:10px; letter-spacing:0.8px; }
        QPushButton { min-height:36px; padding:0 15px; border-radius:2px; font-weight:700; letter-spacing:0.6px; }
        #primaryButton { background:#111111; color:#ffffff; border:1px solid #111111; }
        #primaryButton:hover { background:#333333; }
        #secondaryButton, #outlineButton { background:#ffffff; color:#111111; border:1px solid #111111; }
        #secondaryButton:hover, #outlineButton:hover { background:#e5e5e2; }
        QPushButton:disabled { background:#e6e6e3; color:#9a9a9a; border:1px solid #c9c9c6; }
        QListWidget, QTableWidget { background:#ffffff; border:1px solid #b8b8b5; alternate-background-color:#f2f2ef; }
        QListWidget::item { padding:7px; border-bottom:1px solid #e1e1df; }
        QHeaderView::section { background:#111111; color:#ffffff; border:0; padding:9px; font-size:11px; font-weight:800; }
        QSpinBox, QDoubleSpinBox { min-height:31px; background:#ffffff; border:1px solid #9a9a98; padding:0 8px; }
        QProgressBar { min-height:8px; max-height:8px; background:#ddddda; border:0; }
        QProgressBar::chunk { background:#111111; }
        QSplitter::handle { background:#c8c8c5; width:1px; }
    )"));
}

void MainWindow::browseVisible()
{
    const QString path = QFileDialog::getExistingDirectory(this, QStringLiteral("Chọn thư mục RGB"));
    if (!path.isEmpty()) visibleDrop_->setPath(path);
}

void MainWindow::browseThermal()
{
    const QString path = QFileDialog::getExistingDirectory(this, QStringLiteral("Chọn thư mục ảnh nhiệt"));
    if (!path.isEmpty()) thermalDrop_->setPath(path);
}

void MainWindow::browseRoot()
{
    const QString root = QFileDialog::getExistingDirectory(this, QStringLiteral("Chọn thư mục gốc dataset"));
    if (root.isEmpty()) return;
    QString visible;
    QString thermal;
    if (!DatasetPairer::detectDatasetRoot(root, &visible, &thermal, &datasetName_)) {
        QMessageBox::warning(this, QStringLiteral("Không nhận dạng được"),
            QStringLiteral("Không thấy cấu trúc LLVIP hoặc RoadScene. Hãy thả riêng hai thư mục RGB và IR."));
        return;
    }
    visibleDrop_->setPath(visible);
    thermalDrop_->setPath(thermal);
}

void MainWindow::validateDataset()
{
    if (visibleDrop_->path().isEmpty() || thermalDrop_->path().isEmpty()) return;
    try {
        QStringList warnings;
        const QVector<ImagePair> pairs = DatasetPairer::pairDirectories(
            visibleDrop_->path(), thermalDrop_->path(), &warnings);
        datasetStatus_->setText(QStringLiteral("OK  ·  %1 cặp hợp lệ%2")
            .arg(pairs.size())
            .arg(warnings.isEmpty() ? QString() : QStringLiteral("  ·  ") + warnings.join(QStringLiteral(" "))));
    } catch (const std::exception &error) {
        datasetStatus_->setText(QStringLiteral("LỖI  ·  ") + QString::fromUtf8(error.what()));
    }
}

QStringList MainWindow::selectedMethods() const
{
    QStringList methods;
    for (int i = 0; i < methodList_->count(); ++i) {
        if (methodList_->item(i)->checkState() == Qt::Checked)
            methods.append(methodList_->item(i)->text());
    }
    return methods;
}

void MainWindow::startBenchmark()
{
    if (selectedMethods().isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("Thiếu phương pháp"), QStringLiteral("Chọn ít nhất một phương pháp."));
        return;
    }
    try {
        DatasetPairer::pairDirectories(visibleDrop_->path(), thermalDrop_->path());
    } catch (const std::exception &error) {
        QMessageBox::warning(this, QStringLiteral("Dữ liệu chưa hợp lệ"), QString::fromUtf8(error.what()));
        return;
    }

    BenchmarkConfig config;
    config.visibleDir = visibleDrop_->path();
    config.thermalDir = thermalDrop_->path();
    config.datasetName = datasetName_;
    config.methods = selectedMethods();
    config.maxPairs = maxPairs_->value();
    config.correctnessPx = correctness_->value();
    config.ransacPx = ransac_->value();
    config.maxKeypoints = maxKeypoints_->value();
    config.outputDir = QDir::current().filePath(QStringLiteral("runs"));

    resultTable_->setRowCount(0);
    progressBar_->setValue(0);
    openReportButton_->setEnabled(false);
    setRunning(true);

    workerThread_ = new QThread(this);
    worker_ = new BenchmarkWorker(config);
    worker_->moveToThread(workerThread_);
    connect(workerThread_, &QThread::started, worker_, &BenchmarkWorker::run);
    connect(worker_, &BenchmarkWorker::progress, this, &MainWindow::updateProgress);
    connect(worker_, &BenchmarkWorker::completed, this, &MainWindow::showResult);
    connect(worker_, &BenchmarkWorker::failed, this, &MainWindow::showFailure);
    connect(worker_, &BenchmarkWorker::finished, workerThread_, &QThread::quit);
    connect(worker_, &BenchmarkWorker::finished, worker_, &QObject::deleteLater);
    connect(workerThread_, &QThread::finished, workerThread_, &QObject::deleteLater);
    connect(workerThread_, &QThread::finished, this, [this] {
        worker_ = nullptr;
        workerThread_ = nullptr;
        setRunning(false);
    });
    workerThread_->start();
}

void MainWindow::stopBenchmark()
{
    if (worker_) {
        worker_->cancel();
        statusLabel_->setText(QStringLiteral("Đang dừng sau cặp hiện tại…"));
    }
}

void MainWindow::updateProgress(int current, int total, const QString &message)
{
    progressBar_->setRange(0, total);
    progressBar_->setValue(current);
    statusLabel_->setText(QStringLiteral("%1/%2  ·  %3").arg(current).arg(total).arg(message));
}

void MainWindow::showResult(const BenchmarkResult &result)
{
    lastRunDirectory_ = result.runDirectory;
    resultTable_->setRowCount(result.summary.size());
    for (int row = 0; row < result.summary.size(); ++row) {
        const SummaryRow &summary = result.summary.at(row);
        const QStringList values = {
            summary.method,
            QString::number(summary.pairs),
            percent(summary.mean.precision),
            percent(summary.mean.mma3),
            percent(summary.mean.inlierRatio),
            number(summary.mean.medianError),
            percent(summary.mean.coverage),
            number(summary.mean.runtimeMs),
        };
        for (int column = 0; column < values.size(); ++column)
            resultTable_->setItem(row, column, new QTableWidgetItem(values.at(column)));
    }
    statusLabel_->setText(QStringLiteral("HOÀN TẤT  ·  ") + result.runDirectory);
    openReportButton_->setEnabled(true);
}

void MainWindow::showFailure(const QString &message)
{
    statusLabel_->setText(QStringLiteral("LỖI  ·  ") + message);
    QMessageBox::critical(this, QStringLiteral("Benchmark thất bại"), message);
}

void MainWindow::openReport()
{
    if (!lastRunDirectory_.isEmpty())
        QDesktopServices::openUrl(QUrl::fromLocalFile(lastRunDirectory_ + QStringLiteral("/report.html")));
}

void MainWindow::setRunning(bool running)
{
    runButton_->setEnabled(!running);
    stopButton_->setEnabled(running);
}

} // namespace rgbtir

