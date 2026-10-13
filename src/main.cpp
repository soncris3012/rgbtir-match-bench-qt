#include "core/BenchmarkTypes.h"
#include "ui/MainWindow.h"

#include <QApplication>
#include <QDir>
#include <QFont>
#include <QTimer>

int main(int argc, char *argv[])
{
    QApplication application(argc, argv);
    application.setApplicationName(QStringLiteral("RGB–TIR Match Bench"));
    application.setOrganizationName(QStringLiteral("soncris3012"));
    application.setStyle(QStringLiteral("Fusion"));
    qRegisterMetaType<rgbtir::BenchmarkConfig>();
    qRegisterMetaType<rgbtir::BenchmarkResult>();

    QFont font = application.font();
    font.setPointSize(12);
    application.setFont(font);

    rgbtir::MainWindow window;
    window.show();

    const QStringList arguments = application.arguments();
    const int screenshotIndex = arguments.indexOf(QStringLiteral("--screenshot"));
    if (screenshotIndex >= 0 && screenshotIndex + 1 < arguments.size()) {
        const QString outputPath = QDir::current().absoluteFilePath(arguments.at(screenshotIndex + 1));
        QTimer::singleShot(250, &window, [&application, &window, outputPath] {
            window.grab().save(outputPath);
            application.quit();
        });
    }
    return application.exec();
}

