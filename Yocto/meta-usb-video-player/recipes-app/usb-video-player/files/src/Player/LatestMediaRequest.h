#pragma once

#include <QUrl>
#include <QtGlobal>
#include <optional>

/** @brief Keep the latest media selection, without queueing every Next/Previous click. */
class LatestMediaRequest {
public:
    struct Request {
        QUrl source; ///< Empty means release the current stream without starting another.
        quint64 revision; ///< Monotonically increasing intent version.
    };

    /** @brief Replace pending intent; in-flight work must finish before it is consumed. */
    void replace(const QUrl& source) { pending_ = Request{source, ++revision_}; }
    bool pending() const { return pending_.has_value(); } ///< Whether another transition is requested.
    bool current(quint64 revision) const { return revision == revision_; } ///< Reject obsolete completions.
    quint64 revision() const { return revision_; } ///< Current intent version for snapshot validation.
    QUrl source() const { return pending_.has_value() ? pending_->source : QUrl(); } ///< Pending selection.

    /** @brief Consume only the most recent request; caller must first check pending(). */
    Request take() {
        const Request request = pending_.value();
        pending_.reset();
        return request;
    }

private:
    std::optional<Request> pending_; ///< Single-slot mailbox; no unbounded transition queue.
    quint64 revision_ = 0; ///< Invalidates results from an older selection.
};
