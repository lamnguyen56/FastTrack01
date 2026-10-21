#pragma once

#include <QObject>
#include <QFileSystemWatcher>
#include <QTimer>
#include <functional>
#include "UsbPlaylistModel.h"

/** @brief Watches playlist IPC and mount state without performing USB mounting. */
class UsbPlaylistReader : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool mounted READ isMounted NOTIFY mountedChanged)

public:
    using MountProbe = std::function<bool()>; ///< Injectable mount query for host tests.

    /** @brief Configure the publication path, mount root and output model. */
    UsbPlaylistReader(UsbPlaylistModel& model, QString playlistPath, QString mountPath,
                      MountProbe mountProbe = MountProbe(), QObject* parent = nullptr);

    /** @brief Start watching and read an already inserted USB immediately. */
    void start();

    /** @brief Report whether the configured path is an actual mounted filesystem. */
    bool isMounted() const;

    /** @brief Refresh mount state and consume a complete playlist snapshot. */
    void reload();

signals:
    /** @brief Notify the UI when the selected mount appears or disappears. */
    void mountedChanged();
    /** @brief Emit once on a mounted-to-disconnected transition, never at absent startup. */
    void disconnected();

private:
    /** @brief Reattach the file watch after atomic replacement or late file creation. */
    void armWatcher();

    UsbPlaylistModel& model_; ///< Non-owning model; it outlives this reader.
    QString playlistPath_; ///< Newline-delimited UTF-8 publication on local storage.
    QString mountPath_; ///< Only files canonically inside this root are accepted.
    MountProbe mountProbe_; ///< Real QStorageInfo query unless overridden by tests.
    QFileSystemWatcher watcher_; ///< File and parent-directory notifications.
    QTimer debounce_; ///< Coalesces write/rename bursts before reading.
    QTimer poll_; ///< Recovers missed watch events and observes mount changes.
    QByteArray lastPayload_; ///< Avoids restating USB files for unchanged snapshots.
    bool loaded_ = false; ///< Whether lastPayload_ represents an accepted snapshot.
    bool mounted_ = false; ///< Last observed mount state.
};
