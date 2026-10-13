#include "core/DatasetPairer.h"

#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QHash>
#include <QRegularExpression>
#include <algorithm>
#include <stdexcept>

namespace rgbtir {

QStringList DatasetPairer::imageFiles(const QString &directory)
{
    const QFileInfo input(directory);
    const QStringList suffixes = {
        QStringLiteral("jpg"), QStringLiteral("jpeg"), QStringLiteral("png"),
        QStringLiteral("bmp"), QStringLiteral("tif"), QStringLiteral("tiff")};
    if (input.isFile()) {
        if (suffixes.contains(input.suffix().toLower()))
            return {QDir::cleanPath(input.absoluteFilePath())};
        return {};
    }
    QStringList files;
    const QStringList filters = {
        QStringLiteral("*.jpg"), QStringLiteral("*.jpeg"), QStringLiteral("*.png"),
        QStringLiteral("*.bmp"), QStringLiteral("*.tif"), QStringLiteral("*.tiff")};
    QDirIterator it(directory, filters, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext())
        files.append(QDir::cleanPath(it.next()));
    std::sort(files.begin(), files.end(), [](const QString &a, const QString &b) {
        return a.compare(b, Qt::CaseInsensitive) < 0;
    });
    return files;
}

QString DatasetPairer::normalizedKey(const QString &filePath)
{
    QString key = QFileInfo(filePath).completeBaseName().toLower();
    static const QRegularExpression modality(
        QStringLiteral("(^|[_\\-.])(infrared|thermal|ir|visible|visual|vis|vi)($|[_\\-.])"));
    key.replace(modality, QStringLiteral("_"));
    key.remove(QRegularExpression(QStringLiteral("[^a-z0-9]+")));
    return key;
}

QVector<ImagePair> DatasetPairer::pairDirectories(
    const QString &visibleDir, const QString &thermalDir, QStringList *warnings)
{
    const QStringList visible = imageFiles(visibleDir);
    const QStringList thermal = imageFiles(thermalDir);
    if (visible.isEmpty())
        throw std::runtime_error(QStringLiteral("Không tìm thấy ảnh RGB trong: %1").arg(visibleDir).toStdString());
    if (thermal.isEmpty())
        throw std::runtime_error(QStringLiteral("Không tìm thấy ảnh nhiệt trong: %1").arg(thermalDir).toStdString());

    QHash<QString, QString> visibleMap;
    QHash<QString, QString> thermalMap;
    for (const QString &path : visible)
        visibleMap.insert(normalizedKey(path), path);
    for (const QString &path : thermal)
        thermalMap.insert(normalizedKey(path), path);

    QStringList common;
    for (auto it = visibleMap.cbegin(); it != visibleMap.cend(); ++it) {
        if (thermalMap.contains(it.key()))
            common.append(it.key());
    }
    std::sort(common.begin(), common.end());

    QVector<ImagePair> pairs;
    if (!common.isEmpty()) {
        pairs.reserve(common.size());
        for (const QString &key : common)
            pairs.append({key, visibleMap.value(key), thermalMap.value(key)});
    } else if (visible.size() == thermal.size()) {
        if (warnings)
            warnings->append(QStringLiteral("Tên tệp không khớp; ảnh được ghép theo thứ tự sắp xếp."));
        pairs.reserve(visible.size());
        for (qsizetype i = 0; i < visible.size(); ++i)
            pairs.append({QStringLiteral("pair-%1").arg(i + 1, 5, 10, QLatin1Char('0')), visible.at(i), thermal.at(i)});
    } else {
        throw std::runtime_error(
            QStringLiteral("Không thể ghép cặp: %1 ảnh RGB và %2 ảnh nhiệt.")
                .arg(visible.size()).arg(thermal.size()).toStdString());
    }

    if (warnings && pairs.size() < std::min(visible.size(), thermal.size())) {
        warnings->append(QStringLiteral("Chỉ ghép được %1/%2 cặp theo tên.")
            .arg(pairs.size()).arg(std::min(visible.size(), thermal.size())));
    }
    return pairs;
}

bool DatasetPairer::detectDatasetRoot(
    const QString &root, QString *visibleDir, QString *thermalDir, QString *datasetName)
{
    QFileInfo input(root);
    if (!input.exists())
        return false;
    QDir cursor(input.isDir() ? input.absoluteFilePath() : input.absolutePath());
    QStringList roots;
    for (int depth = 0; depth < 5; ++depth) {
        if (!roots.contains(cursor.absolutePath()))
            roots.append(cursor.absolutePath());
        if (!cursor.cdUp())
            break;
    }

    struct Candidate { QString visible; QString thermal; QString name; };
    for (const QString &rootPath : roots) {
        const QStringList bases = {rootPath, QDir(rootPath).filePath(QStringLiteral("LLVIP"))};
        for (const QString &basePath : bases) {
            const QDir base(basePath);
            QVector<Candidate> candidates = {
                {base.filePath(QStringLiteral("visible")), base.filePath(QStringLiteral("infrared")), QStringLiteral("LLVIP")},
                {base.filePath(QStringLiteral("crop_HR_visible")), base.filePath(QStringLiteral("cropinfrared")), QStringLiteral("RoadScene")},
                {base.filePath(QStringLiteral("crop_LR_visible")), base.filePath(QStringLiteral("cropinfrared")), QStringLiteral("RoadScene")},
                {base.filePath(QStringLiteral("rgb")), base.filePath(QStringLiteral("ir")), QStringLiteral("Custom")},
                {base.filePath(QStringLiteral("images/rgb")), base.filePath(QStringLiteral("images/ir")), QStringLiteral("Custom")},
            };
            for (Candidate candidate : candidates) {
                if (!QDir(candidate.visible).exists() || !QDir(candidate.thermal).exists())
                    continue;
                for (const QString &split : {QStringLiteral("test"), QStringLiteral("val"),
                                             QStringLiteral("validation"), QStringLiteral("train")}) {
                    const QString splitVisible = QDir(candidate.visible).filePath(split);
                    const QString splitThermal = QDir(candidate.thermal).filePath(split);
                    if (QDir(splitVisible).exists() && QDir(splitThermal).exists()) {
                        candidate.visible = splitVisible;
                        candidate.thermal = splitThermal;
                        break;
                    }
                }
                if (visibleDir) *visibleDir = QDir::cleanPath(candidate.visible);
                if (thermalDir) *thermalDir = QDir::cleanPath(candidate.thermal);
                if (datasetName) *datasetName = candidate.name;
                return true;
            }
        }
    }
    return false;
}

} // namespace rgbtir

