#include "UsbPlaylistReader.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSet>
#include <QStorageInfo>
#include <utility>
#include "Logger.h"

UsbPlaylistReader::UsbPlaylistReader(UsbPlaylistModel& model, QString playlistPath,
                                   QString mountPath, MountProbe mountProbe, QObject* parent)
    : QObject(parent), model_(model), playlistPath_(std::move(playlistPath)),
      mountPath_(QDir::cleanPath(mountPath)), mountProbe_(std::move(mountProbe)) {
    if (static_cast<bool>(mountProbe_) == false) {
        mountProbe_ = [this]() {
            QStorageInfo storage(mountPath_);
            storage.refresh();
            return storage.isValid() == true && storage.isReady() == true
                && QDir::cleanPath(storage.rootPath()) == mountPath_;
        };
    }

    debounce_.setSingleShot(true);
    debounce_.setInterval(100);
    poll_.setInterval(1000);
    connect(&watcher_, &QFileSystemWatcher::fileChanged, this, [this]() { debounce_.start(); });
    connect(&watcher_, &QFileSystemWatcher::directoryChanged, this, [this]() { debounce_.start(); });
    connect(&debounce_, &QTimer::timeout, this, &UsbPlaylistReader::reload);
    connect(&poll_, &QTimer::timeout, this, &UsbPlaylistReader::reload);
}

void UsbPlaylistReader::start() {
    armWatcher();
    reload();
    poll_.start();
}

bool UsbPlaylistReader::isMounted() const {
    return mounted_;
}

void UsbPlaylistReader::armWatcher() {
    const QString parent = QFileInfo(playlistPath_).absolutePath();
    if (watcher_.directories().contains(parent) == false && QDir(parent).exists() == true) {
        watcher_.addPath(parent);
    }
    if (watcher_.files().contains(playlistPath_) == false && QFileInfo::exists(playlistPath_) == true) {
        watcher_.addPath(playlistPath_);
    }
}

void UsbPlaylistReader::reload() {
    armWatcher();
    const bool mounted = mountProbe_();
    if (mounted_ != mounted) {
        const bool removed = mounted_ == true && mounted == false;
        mounted_ = mounted;
        loaded_ = false;
        emit mountedChanged();
        if (removed == true) {
            emit disconnected();
        }
        LOG_I("USB UI mount state: ", mounted_ ? "mounted" : "disconnected");
    }

    // A stale publication must never survive USB removal.
    if (mounted_ == false) {
        loaded_ = false;
        model_.replaceFiles({});
        return;
    }

    QFile file(playlistPath_);
    if (file.open(QIODevice::ReadOnly) == false || file.size() > 8 * 1024 * 1024) {
        loaded_ = false;
        model_.replaceFiles({});
        return;
    }
    const QByteArray payload = file.readAll();
    if (file.error() != QFileDevice::NoError) {
        loaded_ = false;
        model_.replaceFiles({});
        return;
    }
    if (loaded_ == true && payload == lastPayload_) {
        return;
    }

    QStringList files;
    QSet<QString> seen;
    const QString root = QFileInfo(mountPath_).canonicalFilePath();
    const QStringList lines = QString::fromUtf8(payload).split(QLatin1Char('\n'), Qt::SkipEmptyParts);
    const QSet<QString> extensions = {"mp4", "mkv", "avi", "mov", "webm"};
    for (const QString& line : lines) {
        // Preserve spaces and Unicode in names; the publisher uses LF, not CRLF.
        const QFileInfo entry(line);
        const QString canonical = entry.canonicalFilePath();
        if (QDir::isAbsolutePath(line) == false || entry.isFile() == false
            || entry.isReadable() == false || entry.isSymLink() == true
            || root.isEmpty() == true || canonical.startsWith(root + QLatin1Char('/')) == false
            || extensions.contains(entry.suffix().toLower()) == false || seen.contains(canonical) == true) {
            continue;
        }
        files.append(canonical);
        seen.insert(canonical);
    }

    lastPayload_ = payload;
    loaded_ = true;
    model_.replaceFiles(files);
    LOG_I("USB UI playlist updated: ", files.size(), " media files");
}
