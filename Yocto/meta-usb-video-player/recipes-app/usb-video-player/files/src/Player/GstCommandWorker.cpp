#include "GstCommandWorker.h"
#include "Logger.h"
#include <QMetaObject>
#include <cstdlib>
#include <exception>
#include <utility>

GstCommandWorker::GstCommandWorker(QObject* parent, int timeoutMs)
    : QObject(parent), timeoutMs_(timeoutMs) {
    // The executor receives blocking jobs on GstControl. The facade and watchdog
    // remain on the GUI thread so timeout detection does not depend on that worker.
    executor_ = new QObject;
    executor_->moveToThread(&thread_);
    thread_.setObjectName(QStringLiteral("GstControl"));
    connect(&thread_, &QThread::finished, executor_, &QObject::deleteLater);
    deadline_.setSingleShot(true);
    connect(&deadline_, &QTimer::timeout, this, [this]() {
        LOG_E("GStreamer operation timed out: ", label_.toStdString(),
              "; exiting so systemd can restart the process");
        // Never terminate a QThread or free a pipeline still accessed by its worker.
        // A blocked plugin/driver cannot safely be unwound from another thread.
        std::_Exit(EXIT_FAILURE);
    });
    thread_.start();
}

GstCommandWorker::~GstCommandWorker() {
    shutdown();
}

bool GstCommandWorker::submit(const QString& label, std::function<void()> work,
                              std::function<void()> completed) {
    if (busy_ == true || stopped_ == true) {
        return false;
    }
    // Reserve the single operation slot before queueing work; repeated UI clicks
    // cannot build an unbounded queue of decoder state changes.
    busy_ = true;
    label_ = label;
    deadline_.start(timeoutMs_);
    QMetaObject::invokeMethod(executor_, [this, work = std::move(work), completed = std::move(completed)]() {
        try {
            work();
        } catch (const std::exception& error) {
            LOG_E("Unhandled media worker exception: ", error.what());
            std::_Exit(EXIT_FAILURE);
        } catch (...) {
            LOG_E("Unhandled media worker exception");
            std::_Exit(EXIT_FAILURE);
        }
        // Return to the facade thread before touching its timer or notifying QML.
        QMetaObject::invokeMethod(this, [this, completed]() {
            deadline_.stop();
            busy_ = false;
            if (stopped_ == false) {
                completed();
            }
        }, Qt::QueuedConnection);
    }, Qt::QueuedConnection);
    return true;
}

void GstCommandWorker::shutdown(std::function<void()> cleanup) {
    if (stopped_ == true) {
        return;
    }
    stopped_ = true;
    deadline_.stop();
    // Queue cleanup after the current operation, never concurrently with it.
    QMetaObject::invokeMethod(executor_, [this, cleanup = std::move(cleanup)]() {
        if (cleanup) cleanup();
        thread_.quit();
    }, Qt::QueuedConnection);
    if (thread_.wait(static_cast<unsigned long>(timeoutMs_)) == false) {
        LOG_E("GStreamer worker did not stop during shutdown; exiting without unsafe cleanup");
        std::_Exit(EXIT_FAILURE);
    }
    executor_ = nullptr;
    busy_ = false;
}
