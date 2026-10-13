#pragma once

#include "core/BenchmarkTypes.h"

namespace rgbtir {

class DatasetPairer {
public:
    static QVector<ImagePair> pairDirectories(
        const QString &visibleDir,
        const QString &thermalDir,
        QStringList *warnings = nullptr);

    static bool detectDatasetRoot(
        const QString &root,
        QString *visibleDir,
        QString *thermalDir,
        QString *datasetName);

private:
    static QString normalizedKey(const QString &filePath);
    static QStringList imageFiles(const QString &directory);
};

} // namespace rgbtir

