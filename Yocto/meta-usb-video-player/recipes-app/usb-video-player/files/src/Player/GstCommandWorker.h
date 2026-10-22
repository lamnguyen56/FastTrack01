#pragma once

#include <QObject>
#include <QThread>
#include <QTimer>
#include <QString>
#include <functional>

/** @brief Execute at most one blocking media operation away from the GUI thread. */
class GstCommandWorker : public QObject {
public:
    /** @brief Start a private executor; timeout includes blocked driver/plugin calls. */
    explicit GstCommandWorker(QObject* parent = nullptr, int timeoutMs = 15000);
    /** @brief Join the executor before its captured session can be destroyed. */
    ~GstCommandWorker() override;

    /** @brief Reject a second operation instead of accumulating obsolete transitions. */
    bool submit(const QString& label, std::function<void()> work, std::function<void()> completed);
    bool busy() const { return busy_; } ///< GUI-thread-only operation state.
    /** @brief Drain the final operation and stop the executor during application teardown. */
    void shutdown(std::function<void()> cleanup = {});

private:
    QThread thread_; ///< Owns the executor event loop, not any Qt Quick item.
    QObject* executor_ = nullptr; ///< Deleted in its own thread when it exits.
    QTimer deadline_; ///< GUI watchdog; cannot interrupt or repair a stuck kernel call.
    QString label_; ///< Included in timeout diagnostics.
    int timeoutMs_; ///< Upper bound before controlled process exit.
    bool busy_ = false; ///< No other operation may touch the pipeline while true.
    bool stopped_ = false; ///< Reject further work during shutdown.
};
