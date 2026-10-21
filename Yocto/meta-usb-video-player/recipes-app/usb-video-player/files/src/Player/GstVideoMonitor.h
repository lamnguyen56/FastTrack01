#pragma once

#include <QElapsedTimer>
#include <gst/gst.h>
#include "VideoFrameProgress.h"

/** @brief Observe actual sink progress without touching Qt objects from streaming threads. */
class GstVideoMonitor {
public:
    /** @brief Bind a non-owning sink; its owner must keep it alive during polling. */
    void attach(GstElement* sink);

    /** @brief Forget measurements when selecting, stopping or removing media. */
    void reset();

    /** @brief Snapshot counters after preroll, before starting the playback clock. */
    void begin();

    /** @brief Read sink progress on the Qt thread and rate-limit stalled-video warnings. */
    bool poll(bool playing);

private:
    /** @brief Read GstBaseSink statistics, returning false if the property is unavailable. */
    bool readCounters(guint64& rendered, guint64& dropped) const;

    GstElement* sink_ = nullptr; ///< Non-owning qmlglsink, owned by GstPlayerBackend.
    QElapsedTimer progressClock_; ///< Time since video progress; not a recovery/retry timer.
    VideoFrameProgress frames_; ///< Per-URI first-frame accounting, independent of PLAYING.
    guint64 dropped_ = 0; ///< Previous drop counter for diagnostic deltas.
};
