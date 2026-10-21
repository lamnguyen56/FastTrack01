#pragma once

#include <QObject>
#include <QUrl>
#include "Usb/UsbPlaylistModel.h"

/** @brief Coordinates selection and transport commands independently of the QML renderer. */
class PlayerController : public QObject {
    Q_OBJECT
    Q_PROPERTY(QUrl source READ source NOTIFY sourceChanged)
    Q_PROPERTY(int currentIndex READ currentIndex NOTIFY selectionChanged)
    Q_PROPERTY(QString currentTitle READ currentTitle NOTIFY selectionChanged)
    Q_PROPERTY(qint64 position READ position NOTIFY timelineChanged)
    Q_PROPERTY(qint64 duration READ duration NOTIFY timelineChanged)
    Q_PROPERTY(bool playing READ isPlaying NOTIFY playbackChanged)
    Q_PROPERTY(bool seekable READ isSeekable NOTIFY playbackChanged)
    Q_PROPERTY(QString errorText READ errorText NOTIFY errorChanged)

public:
    /** @brief Bind transport state to a model that outlives this controller. */
    explicit PlayerController(UsbPlaylistModel& model, QObject* parent = nullptr);

    QUrl source() const; ///< Current local-file URL, or empty when no media exists.
    int currentIndex() const; ///< Selected row, or -1 for an empty model.
    QString currentTitle() const; ///< Display filename of the selected track.
    qint64 position() const; ///< Actual decoder position in milliseconds.
    qint64 duration() const; ///< Decoder duration in milliseconds; zero until known.
    bool isPlaying() const; ///< True only after the media engine reports playing.
    bool isSeekable() const; ///< Whether the active media engine supports seeking.
    QString errorText() const; ///< Current decoder/backend error shown on the UI.

    /** @brief Select a valid row and request playback; invalid rows are ignored. */
    Q_INVOKABLE void selectTrack(int index);
    /** @brief Play or resume the current track if one exists. */
    Q_INVOKABLE void play();
    /** @brief Pause the active media engine. */
    Q_INVOKABLE void pause();
    /** @brief Toggle according to the media engine's actual state. */
    Q_INVOKABLE void toggle();
    /** @brief Select the next row, wrapping at the actual model size, and play. */
    Q_INVOKABLE void next();
    /** @brief Select the previous row, wrapping at the actual model size, and play. */
    Q_INVOKABLE void previous();
    /** @brief Seek by signed seconds, clamped to the known media duration. */
    Q_INVOKABLE void seekRelative(int seconds);
    /** @brief Seek to milliseconds when seeking is supported. */
    Q_INVOKABLE void seekTo(qint64 milliseconds);
    /** @brief Receive the media engine's real position. */
    Q_INVOKABLE void reportPosition(qint64 milliseconds);
    /** @brief Receive the media engine's real duration. */
    Q_INVOKABLE void reportDuration(qint64 milliseconds);
    /** @brief Receive actual playing and seekable flags from the engine. */
    Q_INVOKABLE void reportPlayback(bool playing, bool seekable);
    /** @brief Publish a media/backend failure without pretending playback succeeded. */
    Q_INVOKABLE void reportError(const QString& message);
    /** @brief Format milliseconds as mm:ss or hh:mm:ss. */
    Q_INVOKABLE QString formatTime(qint64 milliseconds) const;

signals:
    void sourceChanged(); ///< Reload the media engine only when the URL changes.
    void selectionChanged(); ///< Update selected row and title.
    void timelineChanged(); ///< Update position and duration bindings.
    void playbackChanged(); ///< Update actual playing and seeking capabilities.
    void errorChanged(); ///< Update the visible error text.
    void playRequested(); ///< Ask the QML media engine to play.
    void pauseRequested(); ///< Ask the QML media engine to pause.
    void stopRequested(); ///< Stop before releasing or replacing a source.
    void seekRequested(qint64 milliseconds); ///< Seek the active media engine.

private:
    /** @brief Preserve a surviving track, otherwise select the first row without autoplay. */
    void synchronizePlaylist();
    /** @brief Replace the selected URL and clear stale decoder metadata. */
    void setSelection(int index);

    UsbPlaylistModel& model_; ///< Non-owning ordered media model.
    QUrl source_; ///< URL used by the selected QML media engine.
    int currentIndex_ = -1; ///< Empty selection sentinel.
    qint64 position_ = 0; ///< Last reported playback position.
    qint64 duration_ = 0; ///< Last reported media duration.
    bool playing_ = false; ///< Last reported media state.
    bool seekable_ = false; ///< Last reported seeking capability.
    QString errorText_; ///< Current backend error; cleared when choosing media.
};
