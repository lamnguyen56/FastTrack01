#ifdef NDEBUG
#undef NDEBUG
#endif
#include <QCoreApplication>
#include <QEventLoop>
#include <QProcess>
#include <QTimer>
#include <cassert>
#include <chrono>
#include <thread>
#include "Player/GstCommandWorker.h"
#include "Player/LatestMediaRequest.h"

/** @brief Exercise the real executor and latest-intent mailbox without decoder/GL dependencies. */
int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    if (app.arguments().contains(QStringLiteral("--timeout"))) {
        GstCommandWorker worker(nullptr, 80);
        assert(worker.submit("test blocked operation", []() {
            std::this_thread::sleep_for(std::chrono::seconds(10));
        }, []() {}));
        return app.exec(); // Watchdog must exit before the blocked work returns.
    }

    LatestMediaRequest requests;
    requests.replace(QUrl("file:///one.mp4"));
    requests.replace(QUrl("file:///two.mp4"));
    requests.replace(QUrl("file:///three.mp4"));
    const auto first = requests.take();
    assert(first.source == QUrl("file:///three.mp4"));
    assert(requests.pending() == false);
    requests.replace(QUrl("file:///four.mp4"));
    requests.replace(QUrl("file:///five.mp4"));
    assert(requests.current(first.revision) == false); // Never start an obsolete preroll.
    const auto latest = requests.take();
    assert(latest.source == QUrl("file:///five.mp4"));
    assert(requests.current(latest.revision) == true);
    requests.replace({});
    assert(requests.take().source.isEmpty() == true); // USB removal wins over pending Play.

    GstCommandWorker worker(nullptr, 1500);
    const auto guiThread = std::this_thread::get_id();
    bool differentThread = false;
    bool completed = false;
    int heartbeats = 0;
    QEventLoop loop;
    QTimer heartbeat;
    heartbeat.setInterval(10);
    QObject::connect(&heartbeat, &QTimer::timeout, [&]() { ++heartbeats; });
    heartbeat.start();
    requests.replace(QUrl("file:///in-flight.mp4"));
    const auto active = requests.take();
    QTimer::singleShot(20, [&]() { requests.replace(QUrl("file:///discard.mp4")); });
    QTimer::singleShot(40, [&]() { requests.replace(QUrl("file:///keep.mp4")); });
    assert(worker.submit("slow reset", [&]() {
        differentThread = std::this_thread::get_id() != guiThread;
        std::this_thread::sleep_for(std::chrono::milliseconds(120));
    }, [&]() {
        assert(std::this_thread::get_id() == guiThread);
        completed = true;
        loop.quit();
    }));
    assert(worker.busy() == true);
    assert(worker.submit("must not queue", []() { assert(false); }, []() {}) == false);
    QTimer::singleShot(1000, &loop, &QEventLoop::quit);
    loop.exec();
    assert(completed == true && differentThread == true);
    assert(worker.busy() == false);
    assert(requests.current(active.revision) == false);
    assert(requests.take().source == QUrl("file:///keep.mp4"));
    assert(heartbeats >= 3); // A blocked operation must not block GUI event dispatch.
    bool cleaned = false;
    worker.shutdown([&]() { cleaned = true; });
    assert(cleaned == true);
    assert(worker.submit("after shutdown", []() {}, []() {}) == false);

    // Shutdown must run cleanup after current work, not skip it when quitting QThread.
    {
        GstCommandWorker finalizer(nullptr, 1500);
        bool workDone = false;
        bool cleanupDone = false;
        bool callbackRan = false;
        assert(finalizer.submit("shutdown in flight", [&]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            workDone = true;
        }, [&]() { callbackRan = true; }));
        finalizer.shutdown([&]() {
            assert(workDone == true);
            cleanupDone = true;
        });
        assert(cleanupDone == true);
        QCoreApplication::processEvents();
        assert(callbackRan == false); // Queued completion must not revive a closing backend.
    }

    // Validate fail-fast behavior in a child, so a stuck decoder cannot hang ctest.
    QProcess child;
    child.start(QCoreApplication::applicationFilePath(), {QStringLiteral("--timeout")});
    assert(child.waitForStarted(3000));
    assert(child.waitForFinished(3000));
    assert(child.exitStatus() == QProcess::NormalExit && child.exitCode() == 1);
    assert(child.readAllStandardOutput().contains("GStreamer operation timed out"));
}
