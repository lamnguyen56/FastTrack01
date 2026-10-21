#include "GstVideoMonitor.h"
#include "Logger.h"

void GstVideoMonitor::attach(GstElement* sink) {
    sink_ = sink;
    reset();
}

void GstVideoMonitor::reset() {
    frames_.reset();
    dropped_ = 0;
    progressClock_.invalidate();
}

bool GstVideoMonitor::readCounters(guint64& rendered, guint64& dropped) const {
    if (sink_ == nullptr) {
        return false;
    }
    GstStructure* stats = nullptr;
    g_object_get(sink_, "stats", &stats, nullptr);
    if (stats == nullptr) {
        return false;
    }
    const bool valid = gst_structure_get_uint64(stats, "rendered", &rendered) != FALSE
                       && gst_structure_get_uint64(stats, "dropped", &dropped) != FALSE;
    gst_structure_free(stats);
    return valid;
}

void GstVideoMonitor::begin() {
    reset();
    guint64 baseline = 0;
    readCounters(baseline, dropped_);
    frames_.reset(baseline);
    progressClock_.start();

    if (sink_ == nullptr) {
        return;
    }

    // Negotiated caps show what reached the GL sink, not whether pixels are visible.
    GstPad* pad = gst_element_get_static_pad(sink_, "sink");
    GstCaps* caps = pad != nullptr ? gst_pad_get_current_caps(pad) : nullptr;
    if (caps != nullptr) {
        gchar* text = gst_caps_to_string(caps);
        LOG_I("Video sink negotiated: ", text);
        g_free(text);
        gst_caps_unref(caps);
    }
    if (pad != nullptr) {
        gst_object_unref(pad);
    }
}

bool GstVideoMonitor::poll(bool playing) {
    guint64 rendered = 0;
    guint64 dropped = 0;
    if (readCounters(rendered, dropped) == false) {
        return frames_.started();
    }
    const bool hadFrame = frames_.started();
    if (frames_.observe(rendered) == true) {
        if (hadFrame == false) {
            LOG_I("Video sink rendered first buffer; rendered=", rendered, "; dropped=", dropped);
        }
        progressClock_.restart();
    } else if (playing == false) {
        // Pause/held EOS is intentional and must never produce stall warnings.
        progressClock_.restart();
    } else if (progressClock_.isValid() == true && progressClock_.elapsed() >= 3000) {
        LOG_W("Video sink has no frame progress for 3s; rendered=", rendered,
              "; dropped=", dropped, "; drops since last poll=", dropped >= dropped_ ? dropped - dropped_ : dropped);
        progressClock_.restart();
    }
    dropped_ = dropped;
    return frames_.started();
}
