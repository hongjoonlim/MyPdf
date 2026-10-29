#include "toolpaths.h"
#include "build_config.h"
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>

QString Tools::qpdf(const QString &applicationDirectory) {
    const QString overridePath=qEnvironmentVariable("MYPDF_QPDF");
    if (!overridePath.isEmpty()) return overridePath;
    const QDir directory(applicationDirectory.isEmpty() ? QCoreApplication::applicationDirPath() : applicationDirectory);
#ifdef Q_OS_WIN
    const QString name=QStringLiteral("qpdf.exe");
#else
    const QString name=QStringLiteral("qpdf");
#endif
    const QStringList candidates{directory.filePath(name),directory.filePath("../Helpers/"+name)};
    for (const auto &candidate:candidates) {
        const QFileInfo file(candidate);
        if (file.isFile() && file.isExecutable()) return file.absoluteFilePath();
    }
    const QString fromPath=QStandardPaths::findExecutable(name);
    if (!fromPath.isEmpty()) return fromPath;
    const QFileInfo configured(QString::fromUtf8(MYPDF_CONFIGURED_QPDF));
    if (configured.isFile() && configured.isExecutable()) return configured.absoluteFilePath();
    return {};
}
