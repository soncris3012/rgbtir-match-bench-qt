#include "ui/MainWindow.h"

#include "core/DatasetPairer.h"
#include "core/FeatureMatcher.h"
#include "ui/DropZone.h"
#include "worker/BenchmarkWorker.h"

#include <QDesktopServices>
#include <QDir>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QPixmap>
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

QLabel *previewLabel(QWidget *parent)
{
    auto *label = new QLabel(QStringLiteral("Chưa có ảnh"), parent);
    label->setObjectName(QStringLiteral("imagePreview"));
    label->setAlignment(Qt::AlignCenter);
    label->setMinimumHeight(210);
    label->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
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
    setWindowTitle(QStringLiteral("RGB–TIR Match Bench — Qt/C++"));
    resize(1380, 860);
    setMinimumSize(1080, 720);

    auto *central = new QWidget(this);
    setCentralWidget(central);
    auto *outer = new QVBoxLayout(central);
    outer->setContentsMargins(8, 8, 8, 8);
    outer->setSpacing(8);

    auto *header = new QHBoxLayout;
    auto *titleColumn = new QVBoxLayout;
    auto *title = new QLabel(QStringLiteral("RGB–TIR MATCH BENCH"), central);
    title->setObjectName(QStringLiteral("appTitle"));
    auto *subtitle = new QLabel(
        QStringLiteral("C++17 · Qt 6 · OpenCV  |  LLVIP · RoadScene · ảnh ghép tùy chọn"), central);
    subtitle->setObjectName(QStringLiteral("subtitle"));
    titleColumn->addWidget(title);
    titleColumn->addWidget(subtitle);
    datasetType_ = new QLabel(QStringLiteral("DATASET: CHƯA CHỌN"), central);
    datasetType_->setObjectName(QStringLiteral("datasetBadge"));
    header->addLayout(titleColumn, 1);
    header->addWidget(datasetType_, 0, Qt::AlignBottom);
    outer->addLayout(header);

    auto *splitter = new QSplitter(Qt::Horizontal, central);
    splitter->setChildrenCollapsible(false);
    outer->addWidget(splitter, 1);
    auto *left = new QWidget(splitter);
    left->setMinimumWidth(350);
    left->setMaximumWidth(390);
    auto *right = new QWidget(splitter);
    splitter->addWidget(left);
    splitter->addWidget(right);
    splitter->setSizes({370, 990});

    auto *leftLayout = new QVBoxLayout(left);
    leftLayout->setContentsMargins(0, 0, 8, 0);
    leftLayout->setSpacing(8);

    auto *dataGroup = new QGroupBox(QStringLiteral("Dữ liệu đầu vào"), left);
    auto *dataLayout = new QVBoxLayout(dataGroup);
    auto *rootRow = new QHBoxLayout;
    datasetRoot_ = new QLineEdit(dataGroup);
    datasetRoot_->setPlaceholderText(QStringLiteral("Thư mục gốc LLVIP / RoadScene"));
    auto *rootButton = new QPushButton(QStringLiteral("MỞ…"), dataGroup);
    rootButton->setObjectName(QStringLiteral("smallButton"));
    rootRow->addWidget(datasetRoot_, 1);
    rootRow->addWidget(rootButton);
    dataLayout->addLayout(rootRow);
    visibleDrop_ = new DropZone(QStringLiteral("RGB / VISIBLE"), QStringLiteral("Chưa chọn ảnh hoặc thư mục"), dataGroup);
    thermalDrop_ = new DropZone(QStringLiteral("THERMAL / IR"), QStringLiteral("Chưa chọn ảnh hoặc thư mục"), dataGroup);
    dataLayout->addWidget(visibleDrop_);
    dataLayout->addWidget(thermalDrop_);
    datasetStatus_ = new QLabel(
        QStringLiteral("Mở thư mục gốc, mở từng ảnh, hoặc kéo-thả trực tiếp."), dataGroup);
    datasetStatus_->setObjectName(QStringLiteral("datasetStatus"));
    datasetStatus_->setWordWrap(true);
    dataLayout->addWidget(datasetStatus_);
    leftLayout->addWidget(dataGroup);

    auto *methodGroup = new QGroupBox(QStringLiteral("Phương pháp"), left);
    auto *methodLayout = new QVBoxLayout(methodGroup);
    methodList_ = new QListWidget(methodGroup);
    methodList_->setObjectName(QStringLiteral("methodList"));
    methodList_->setSelectionMode(QAbstractItemView::NoSelection);
    const QStringList methods = FeatureMatcherFactory::availableMethods();
    for (int i = 0; i < methods.size(); ++i) {
        auto *item = new QListWidgetItem(methods.at(i), methodList_);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(i < 2 ? Qt::Checked : Qt::Unchecked);
    }
    methodLayout->addWidget(methodList_);
    leftLayout->addWidget(methodGroup, 1);

    auto *parameterGroup = new QGroupBox(QStringLiteral("Tham số benchmark"), left);
    auto *form = new QFormLayout(parameterGroup);
    form->setHorizontalSpacing(14);
    maxPairs_ = new QSpinBox(parameterGroup);
    maxPairs_->setRange(1, 100000);
    maxPairs_->setValue(100);
    correctness_ = new QDoubleSpinBox(parameterGroup);
    correctness_->setRange(0.1, 50.0);
    correctness_->setValue(5.0);
    correctness_->setSuffix(QStringLiteral(" px"));
    ransac_ = new QDoubleSpinBox(parameterGroup);
    ransac_->setRange(0.1, 50.0);
    ransac_->setValue(3.0);
    ransac_->setSuffix(QStringLiteral(" px"));
    maxKeypoints_ = new QSpinBox(parameterGroup);
    maxKeypoints_->setRange(128, 16384);
    maxKeypoints_->setSingleStep(128);
    maxKeypoints_->setValue(2048);
    form->addRow(QStringLiteral("Số cặp tối đa"), maxPairs_);
    form->addRow(QStringLiteral("Ngưỡng đúng"), correctness_);
    form->addRow(QStringLiteral("RANSAC"), ransac_);
    form->addRow(QStringLiteral("Keypoint tối đa"), maxKeypoints_);
    leftLayout->addWidget(parameterGroup);

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
    rightLayout->setContentsMargins(8, 0, 0, 0);
    rightLayout->setSpacing(8);

    auto *previewGroup = new QGroupBox(QStringLiteral("Xem trước cặp ảnh"), right);
    auto *previewLayout = new QGridLayout(previewGroup);
    visibleName_ = new QLabel(QStringLiteral("RGB / Visible"), previewGroup);
    thermalName_ = new QLabel(QStringLiteral("Thermal / IR"), previewGroup);
    visibleName_->setObjectName(QStringLiteral("previewName"));
    thermalName_->setObjectName(QStringLiteral("previewName"));
    visiblePreview_ = previewLabel(previewGroup);
    thermalPreview_ = previewLabel(previewGroup);
    previewLayout->addWidget(visibleName_, 0, 0);
    previewLayout->addWidget(thermalName_, 0, 1);
    previewLayout->addWidget(visiblePreview_, 1, 0);
    previewLayout->addWidget(thermalPreview_, 1, 1);
    previewLayout->setColumnStretch(0, 1);
    previewLayout->setColumnStretch(1, 1);
    rightLayout->addWidget(previewGroup, 2);

    auto *resultGroup = new QGroupBox(QStringLiteral("Kết quả tổng hợp"), right);
    auto *resultLayout = new QVBoxLayout(resultGroup);
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
    resultLayout->addWidget(resultTable_);
    rightLayout->addWidget(resultGroup, 2);

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
    openRunDirectoryButton_ = new QPushButton(QStringLiteral("MỞ THƯ MỤC KẾT QUẢ"), right);
    openRunDirectoryButton_->setObjectName(QStringLiteral("outlineButton"));
    openRunDirectoryButton_->setEnabled(false);
    auto *resultButtons = new QHBoxLayout;
    resultButtons->addWidget(openReportButton_);
    resultButtons->addWidget(openRunDirectoryButton_);
    resultButtons->addStretch();
    rightLayout->addWidget(progressBar_);
    rightLayout->addWidget(statusLabel_);
    rightLayout->addLayout(resultButtons);

    connect(rootButton, &QPushButton::clicked, this, &MainWindow::chooseDatasetRoot);
    connect(datasetRoot_, &QLineEdit::editingFinished, this, [this] {
        if (!datasetRoot_->text().trimmed().isEmpty()) {
            visibleDrop_->setPath(datasetRoot_->text().trimmed());
        }
    });
    connect(visibleDrop_, &DropZone::pathChanged, this, &MainWindow::validateDataset);
    connect(thermalDrop_, &DropZone::pathChanged, this, &MainWindow::validateDataset);
    connect(runButton_, &QPushButton::clicked, this, &MainWindow::startBenchmark);
    connect(stopButton_, &QPushButton::clicked, this, &MainWindow::stopBenchmark);
    connect(openReportButton_, &QPushButton::clicked, this, &MainWindow::openReport);
    connect(openRunDirectoryButton_, &QPushButton::clicked, this, &MainWindow::openRunDirectory);
}

void MainWindow::applyMonochromeStyle()
{
    setStyleSheet(QStringLiteral(R"(
        * { color:#e2e2e2; font-size:13px; }
        QMainWindow, QWidget { background:#2b2b2b; }
        #appTitle { font-size:25px; font-weight:800; letter-spacing:1px; color:#ffffff; }
        #subtitle, #datasetStatus, #statusLabel { color:#aaaaaa; }
        #datasetBadge { background:#202020; border:1px solid #777777; padding:6px 10px; font-weight:700; }
        QGroupBox { background:#303030; border:1px solid #707070; margin-top:10px; padding:9px 5px 5px 5px; font-weight:700; }
        QGroupBox::title { subcontrol-origin:margin; left:7px; padding:0 4px; }
        #dropZone { background:#242424; border:1px dashed #777777; min-height:122px; }
        #dropZone:hover, #dropZone[active="true"] { border:1px solid #f0f0f0; background:#333333; }
        #dropTitle, #previewName { font-weight:800; color:#f3f3f3; }
        #dropValue { color:#bdbdbd; }
        #dropCaption { color:#888888; font-size:10px; letter-spacing:0.6px; }
        #imagePreview { background:#181818; border:1px solid #666666; color:#777777; }
        QLineEdit, QSpinBox, QDoubleSpinBox { min-height:28px; background:#202020; border:1px solid #777777; padding:0 7px; }
        QLineEdit:focus, QSpinBox:focus, QDoubleSpinBox:focus { border:1px solid #d0d0d0; }
        QPushButton { min-height:32px; padding:0 12px; border:1px outset #888888; background:#444444; font-weight:700; }
        QPushButton:pressed { border-style:inset; background:#333333; }
        #smallButton { min-height:27px; padding:0 8px; font-size:11px; }
        #primaryButton { background:#3b4c62; color:#ffffff; border:1px solid #8b9bb0; }
        #primaryButton:hover { background:#485e79; }
        #secondaryButton, #outlineButton { background:#3b3b3b; border:1px solid #888888; }
        #secondaryButton:hover, #outlineButton:hover { background:#505050; }
        QPushButton:disabled { color:#777777; background:#383838; border:1px solid #555555; }
        QListWidget, QTableWidget { background:#252525; border:1px solid #707070; alternate-background-color:#2f2f2f; gridline-color:#555555; selection-background-color:#385478; }
        QListWidget::item { padding:6px; border-bottom:1px solid #474747; }
        QHeaderView::section { background:#444444; color:#f5f5f5; border:1px solid #606060; padding:6px; font-size:11px; font-weight:800; }
        QProgressBar { min-height:16px; max-height:16px; background:#202020; border:1px solid #707070; }
        QProgressBar::chunk { background:#4c76a8; }
        QSplitter::handle { background:#5b5b5b; width:1px; }
    )"));
}

void MainWindow::chooseDatasetRoot()
{
    const QString root = QFileDialog::getExistingDirectory(
        this,
        QStringLiteral("Chọn thư mục gốc LLVIP / RoadScene"),
        datasetRoot_->text(),
        QFileDialog::ShowDirsOnly | QFileDialog::DontUseNativeDialog);
    if (root.isEmpty())
        return;
    datasetRoot_->setText(root);
    QString visible;
    QString thermal;
    QString name;
    if (DatasetPairer::detectDatasetRoot(root, &visible, &thermal, &name)) {
        setDatasetInputs(visible, thermal, name);
        validateDataset();
    } else {
        datasetStatus_->setText(QStringLiteral(
            "Không nhận ra cấu trúc bộ dữ liệu. Hãy mở riêng thư mục/ảnh RGB và Thermal bên dưới."));
        datasetType_->setText(QStringLiteral("DATASET: TÙY CHỌN"));
    }
}

void MainWindow::setDatasetInputs(
    const QString &visible, const QString &thermal, const QString &name)
{
    updatingInputs_ = true;
    datasetName_ = name;
    visibleDrop_->setPath(visible);
    thermalDrop_->setPath(thermal);
    datasetType_->setText(QStringLiteral("DATASET: %1").arg(name.toUpper()));
    updatingInputs_ = false;
}

void MainWindow::validateDataset()
{
    if (updatingInputs_)
        return;
    if (auto *zone = qobject_cast<DropZone *>(sender())) {
        QString detectedVisible;
        QString detectedThermal;
        QString detectedName;
        if (DatasetPairer::detectDatasetRoot(
                zone->path(), &detectedVisible, &detectedThermal, &detectedName)) {
            setDatasetInputs(detectedVisible, detectedThermal, detectedName);
        }
    }
    if (visibleDrop_->path().isEmpty() || thermalDrop_->path().isEmpty()) {
        datasetStatus_->setText(QStringLiteral("Cần chọn đủ đầu vào RGB và Thermal / IR."));
        return;
    }
    try {
        QStringList warnings;
        const QVector<ImagePair> pairs = DatasetPairer::pairDirectories(
            visibleDrop_->path(), thermalDrop_->path(), &warnings);
        if (QFileInfo(visibleDrop_->path()).isFile() && QFileInfo(thermalDrop_->path()).isFile()) {
            datasetName_ = QStringLiteral("Single pair");
            datasetType_->setText(QStringLiteral("DATASET: CẶP ẢNH ĐƠN"));
            maxPairs_->setValue(1);
        } else if (datasetName_ == QStringLiteral("Auto")) {
            datasetName_ = QStringLiteral("Custom");
            datasetType_->setText(QStringLiteral("DATASET: TÙY CHỌN"));
        }
        datasetStatus_->setText(QStringLiteral("SẴN SÀNG  ·  %1 cặp hợp lệ%2")
            .arg(pairs.size())
            .arg(warnings.isEmpty() ? QString() : QStringLiteral("  ·  ") + warnings.join(QStringLiteral(" "))));
        if (!pairs.isEmpty())
            updatePreview(pairs.first().visiblePath, pairs.first().thermalPath);
    } catch (const std::exception &error) {
        datasetStatus_->setText(QStringLiteral("LỖI  ·  ") + QString::fromUtf8(error.what()));
    }
}

void MainWindow::updatePreview(const QString &visiblePath, const QString &thermalPath)
{
    const auto setImage = [](QLabel *view, QLabel *name, const QString &path) {
        const QPixmap pixmap(path);
        name->setText(QFileInfo(path).fileName());
        name->setToolTip(path);
        if (pixmap.isNull()) {
            view->setPixmap(QPixmap());
            view->setText(QStringLiteral("Không đọc được ảnh"));
            return;
        }
        QSize target = view->size();
        if (target.width() < 100 || target.height() < 100)
            target = QSize(560, 280);
        view->setText(QString());
        view->setPixmap(pixmap.scaled(target, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        view->setToolTip(QStringLiteral("%1 × %2\n%3")
            .arg(pixmap.width()).arg(pixmap.height()).arg(path));
    };
    setImage(visiblePreview_, visibleName_, visiblePath);
    setImage(thermalPreview_, thermalName_, thermalPath);
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
    config.outputDir = QDir(
        QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation))
        .filePath(QStringLiteral("runs"));

    resultTable_->setRowCount(0);
    progressBar_->setValue(0);
    openReportButton_->setEnabled(false);
    openRunDirectoryButton_->setEnabled(false);
    setRunning(true);

    workerThread_ = new QThread(this);
    worker_ = new BenchmarkWorker(config);
    worker_->moveToThread(workerThread_);
    connect(workerThread_, &QThread::started, worker_, &BenchmarkWorker::run);
    connect(worker_, &BenchmarkWorker::progress, this, &MainWindow::updateProgress);
    connect(worker_, &BenchmarkWorker::pairStarted, this, &MainWindow::showCurrentPair);
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

void MainWindow::showCurrentPair(
    const QString &visiblePath,
    const QString &thermalPath,
    const QString &method,
    int current,
    int total)
{
    updatePreview(visiblePath, thermalPath);
    const QString position = QStringLiteral("%1/%2 · %3").arg(current).arg(total).arg(method);
    visibleName_->setText(QStringLiteral("%1  |  %2")
        .arg(QFileInfo(visiblePath).fileName(), position));
    thermalName_->setText(QStringLiteral("%1  |  %2")
        .arg(QFileInfo(thermalPath).fileName(), position));
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
    openRunDirectoryButton_->setEnabled(true);
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

void MainWindow::openRunDirectory()
{
    if (!lastRunDirectory_.isEmpty())
        QDesktopServices::openUrl(QUrl::fromLocalFile(lastRunDirectory_));
}

void MainWindow::setRunning(bool running)
{
    runButton_->setEnabled(!running);
    stopButton_->setEnabled(running);
}

} // namespace rgbtir

