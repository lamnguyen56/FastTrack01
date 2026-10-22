#pragma once

#include <QString>
#include <QUrl>
#include <gst/gst.h>
#include "GstVideoMonitor.h"

/** @brief Immutable result delivered from the media worker to the GUI thread. */
struct GstSessionSnapshot {
    QString error; ///< Backend failure, empty on success.
    qint64 position = 0; ///< Current position in milliseconds.
    qint64 duration = 0; ///< Current duration in milliseconds.
    bool playing = false; ///< Actual pipeline state, not requested state.
    bool seekable = false; ///< Actual media seeking capability.
    bool prerolled = false; ///< Video topology and first preroll are ready.
    bool buffering = false; ///< A non-live stream is waiting for input data.
    bool started = false; ///< Sink counters advanced for this URI.
    bool ended = false; ///< EOS observed; retained until reset/seek.
};

/** @brief Own the pipeline; all methods except construction run on GstControl. */
class GstPipelineSession {
public:
    /** @brief Retain a registered qmlglsink whose QML item is managed by the GUI. */
    explicit GstPipelineSession(GstElement* sink);
    /** @brief References may be released only after release() and worker shutdown. */
    ~GstPipelineSession();

    /** @brief Flush the old stream before READY, then replace its URI. */
    QString reset(const QUrl& source);
    /** @brief Apply a serialized state change after the render-thread READY step. */
    QString setState(GstState state);
    /** @brief Run a flush seek without holding or blocking the GUI event loop. */
    QString seek(qint64 milliseconds);
    /** @brief Query the bus, timeline, preroll and sink progress on the worker. */
    GstSessionSnapshot poll();
    /** @brief Release media handles and GL pipeline references during final shutdown. */
    void release();

private:
    /** @brief Construct playbin and its GL sink bin without starting media. */
    QString create();
    /** @brief Wait for an asynchronous transition on GstControl with a finite deadline. */
    QString transition(GstState state);
    /** @brief Wake blocked streaming/poll tasks before joining them in a state change. */
    void flush();
    /** @brief Drain bus diagnostics, preferring explicit video support errors over secondary stream errors. */
    QString drainBus();

    GstElement* sink_ = nullptr; ///< Owned reference shared with the GUI render job.
    GstElement* pipeline_ = nullptr; ///< Owned persistent playbin.
    GstBus* bus_ = nullptr; ///< Owned bus, polled only by the worker.
    GstVideoMonitor monitor_; ///< No Qt Quick or QObject access.
    bool used_ = false; ///< A stream has entered PAUSED/PLAYING and needs flushing.
    bool loaded_ = false; ///< Current URI is nonempty.
    bool prerolled_ = false; ///< Explicit PAUSED preroll completed.
    bool buffering_ = false; ///< Latest buffering percentage was below 100.
    bool ended_ = false; ///< EOS for the current stream.
};
