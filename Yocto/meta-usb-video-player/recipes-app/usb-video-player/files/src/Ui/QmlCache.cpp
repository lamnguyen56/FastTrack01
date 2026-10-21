#include "QmlCache.h"
#include <QCryptographicHash>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QStandardPaths>
#include <QStringList>
#include "Logger.h"

QString QmlCache::configure(const QString& sourceRoot, const QString& cacheRoot) {
    QStringList sources;
    QDirIterator iterator(sourceRoot, {"*.qml", "*.js", "*.mjs"}, QDir::Files, QDirIterator::Subdirectories);
    while (iterator.hasNext() == true) {
        sources.append(iterator.next());
    }
    sources.sort();
    if (sources.isEmpty() == true) {
        qputenv("QML_DISABLE_DISK_CACHE", "1");
        LOG_W("No QML sources found for cache fingerprint; disk cache disabled");
        return QString();
    }

    QCryptographicHash hash(QCryptographicHash::Sha256);
    for (const QString& source : sources) {
        QFile file(source);
        if (file.open(QIODevice::ReadOnly) == false) {
            qputenv("QML_DISABLE_DISK_CACHE", "1");
            LOG_W("Cannot read QML source for cache fingerprint; disk cache disabled");
            return QString();
        }
        const QByteArray bytes = file.readAll();
        if (file.error() != QFileDevice::NoError) {
            qputenv("QML_DISABLE_DISK_CACHE", "1");
            return QString();
        }
        hash.addData(source.toUtf8());
        hash.addData("\0", 1);
        hash.addData(bytes);
        hash.addData("\0", 1);
    }

    const QString fingerprint = QString::fromLatin1(hash.result().toHex());
    const QString base = cacheRoot.isEmpty() == true
        ? QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + "/qml-by-content"
        : cacheRoot;
    const QString path = QDir(base).filePath(fingerprint);
    if (QDir().mkpath(path) == false) {
        qputenv("QML_DISABLE_DISK_CACHE", "1");
        LOG_W("Cannot create QML cache directory; disk cache disabled");
        return QString();
    }

    // Qt5 caches resource compilations using their URL and source timestamp.
    // Yocto's SOURCE_DATE_EPOCH may keep that timestamp constant across edits.
    qputenv("QML_DISK_CACHE_PATH", path.toUtf8());
    LOG_I("QML UI fingerprint: ", fingerprint.toStdString());
    return path;
}
