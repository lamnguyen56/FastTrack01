// Integration tests use temporary files and an injected mount probe, never real USB hardware.
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <QCoreApplication>
#include <QDir>
#include <QDateTime>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QTemporaryDir>
#include <QThread>
#include <functional>
#include "Usb/UsbPlaylistReader.h"
#include "Player/PlayerController.h"
#include "Ui/QmlCache.h"

static void publish(const QString& path, const QByteArray& bytes) {
    QSaveFile file(path);
    assert(file.open(QIODevice::WriteOnly) == true);
    assert(file.write(bytes) == bytes.size());
    assert(file.commit() == true);
}

static void waitUntil(const std::function<bool()>& ready) {
    QElapsedTimer timer;
    timer.start();
    while (ready() == false && timer.elapsed() < 3000) {
        QCoreApplication::processEvents();
        QThread::msleep(5);
    }
    assert(ready() == true);
}

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    QTemporaryDir temporary;
    assert(temporary.isValid() == true);
    const QString root = temporary.path() + "/usb";
    assert(QDir().mkpath(root) == true);
    const QString first = root + "/clip #1 %.mp4";
    const QString second = root + QString::fromUtf8("/video tiếng Việt.MKV");
    const QString third = root + "/third.webm";
    const QString outside = temporary.path() + "/outside.mp4";
    for (const QString& path : {first, second, third, outside}) {
        publish(path, "dummy media; playback is outside this core test");
    }
    const QString ignored = root + "/notes.txt";
    publish(ignored, "not a media file");
    const QString alias = root + "/alias.mp4";
    QFile::link(first, alias);
    const QString ipc = temporary.path() + "/playlist.txt";
    const QByteArray snapshot = (first + '\n' + second + '\n' + third + '\n').toUtf8();
    publish(ipc, snapshot);

    UsbPlaylistModel model;
    PlayerController player(model);
    bool mounted = false;
    UsbPlaylistReader reader(model, ipc, root, [&mounted]() { return mounted; });
    int plays = 0;
    int stops = 0;
    qint64 seek = -1;
    QObject::connect(&player, &PlayerController::playRequested, [&plays]() { ++plays; });
    QObject::connect(&player, &PlayerController::stopRequested, [&stops]() { ++stops; });
    QObject::connect(&player, &PlayerController::seekRequested, [&seek](qint64 value) { seek = value; });
    reader.start();
    assert(model.count() == 0); // Stale IPC is rejected while the mount is absent.
    assert(player.currentIndex() == -1);

    mounted = true;
    reader.reload();
    assert(model.count() == 3);
    assert(player.currentIndex() == 0);
    assert(player.source().toLocalFile() == first);
    assert(plays == 0); // Inserting USB must not start playback automatically.
    assert(model.data(model.index(1, 0), UsbPlaylistModel::TitleRole).toString() == QFileInfo(second).fileName());
    assert(model.pathAt(-1).isEmpty() == true);

    player.previous();
    assert(player.currentIndex() == 2);
    player.next();
    assert(player.currentIndex() == 0);
    player.selectTrack(1);
    assert(player.source().toLocalFile() == second);
    const int validPlays = plays;
    player.selectTrack(99);
    assert(plays == validPlays);
    player.reportDuration(10000);
    player.reportPosition(5000);
    player.seekRelative(100);
    assert(seek == -1); // Seeking is disabled until reported by the engine.
    player.reportPlayback(true, true);
    player.seekRelative(100);
    assert(seek == 10000);
    player.seekRelative(-100);
    assert(seek == 0);
    assert(player.formatTime(65000) == "01:05");
    assert(player.formatTime(3661000) == "1:01:01");

    player.reportError(QStringLiteral("decoder unavailable"));
    player.play();
    assert(player.errorText().isEmpty() == false);
    player.reportPlayback(true, true);
    assert(player.errorText().isEmpty() == true);

    const int playsBeforeReorder = plays;
    // Atomic replacement preserves the surviving selection even if its row moves.
    publish(ipc, (second + '\n' + first + '\n' + second + '\n' + outside + '\n' + ignored + '\n' + alias + "\nrelative.mp4\n").toUtf8());
    waitUntil([&model]() { return model.count() == 2; });
    assert(player.currentIndex() == 0);
    assert(player.source().toLocalFile() == second);
    assert(player.position() == 5000);
    assert(plays == playsBeforeReorder);

    // Deletion and recreation must not lose the watcher after inode changes.
    assert(QFile::remove(ipc) == true);
    waitUntil([&model]() { return model.count() == 0; });
    assert(player.source().isEmpty() == true);
    assert(player.duration() == 0);
    publish(ipc, (first + '\n').toUtf8());
    waitUntil([&model]() { return model.count() == 1; });
    player.next();
    assert(player.currentIndex() == 0); // A single row wraps safely.

    // An unplug invalidates the playlist even when the publication was not cleared.
    mounted = false;
    waitUntil([&model]() { return model.count() == 0; });
    assert(reader.isMounted() == false);
    assert(player.currentIndex() == -1);
    assert(player.source().isEmpty() == true);
    assert(player.position() == 0);
    assert(stops > 0);

    mounted = true;
    reader.reload();
    assert(model.count() == 1);
    publish(ipc, QByteArray());
    waitUntil([&model]() { return model.count() == 0; });
    assert(reader.isMounted() == true); // Mounted USB with no media differs from unplugged.
    player.next();
    player.previous();
    assert(player.currentIndex() == -1);
    assert(QFile::exists(QStringLiteral(":/ui/main.qml")) == true);
    assert(QFile::exists(QStringLiteral(":/ui/VideoSurfaceQt5.qml")) == true);
    assert(QmlCache::configure(QStringLiteral(":/ui"), temporary.path() + "/resource-cache").isEmpty() == false);

    // Same resource path and timestamp must not reuse a different QML payload.
    const QString qmlRoot = temporary.path() + "/qml";
    assert(QDir().mkpath(qmlRoot) == true);
    const QString qmlFile = qmlRoot + "/main.qml";
    publish(qmlFile, "QtObject { property int revision: 1 }");
    const QDateTime fixedTime = QFileInfo(qmlFile).lastModified();
    const QString cacheRoot = temporary.path() + "/cache";
    const QString firstCache = QmlCache::configure(qmlRoot, cacheRoot);
    assert(firstCache.isEmpty() == false);
    assert(QmlCache::configure(qmlRoot, cacheRoot) == firstCache);
    publish(qmlFile, "QtObject { property int revision: 2 }");
    QFile sourceFile(qmlFile);
    assert(sourceFile.open(QIODevice::ReadWrite) == true);
    assert(sourceFile.setFileTime(fixedTime, QFileDevice::FileModificationTime) == true);
    sourceFile.close();
    const QString secondCache = QmlCache::configure(qmlRoot, cacheRoot);
    assert(secondCache != firstCache);
    assert(qgetenv("QML_DISK_CACHE_PATH") == secondCache.toUtf8());
    assert(QmlCache::configure(qmlRoot + "/missing", cacheRoot).isEmpty() == true);
    assert(qgetenv("QML_DISABLE_DISK_CACHE") == "1");
    return 0;
}
