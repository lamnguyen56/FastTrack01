// Exercise controller-to-backend signals and callback guards without a decoder.
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <QCoreApplication>
#include <QTimer>
#include <QSize>
#include <QVariant>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQmlContext>
#include <QDebug>
#include "Ui/QmlTypes.h"
#include "Player/VideoPlaybackBridge.h"
#include "Ui/UsbNoticeState.h"

class MockMedia : public QObject {
    Q_OBJECT
    Q_PROPERTY(QUrl source MEMBER source)
    Q_PROPERTY(int error MEMBER error)
    Q_PROPERTY(QString errorString MEMBER errorString)
    Q_PROPERTY(QObject* metaData READ metadata)
public:
    QUrl source;
    int error = 0;
    QString errorString;
    int plays = 0;
    int pauses = 0;
    int stops = 0;
    int lastSeek = -1;
    QObject metadataObject;
    QObject* metadata() { return &metadataObject; }
    Q_INVOKABLE void play() { ++plays; }
    Q_INVOKABLE void pause() { ++pauses; }
    Q_INVOKABLE void stop() { ++stops; }
    Q_INVOKABLE void seek(int position) { lastSeek = position; }
};

static void drain() {
    for (int step = 0; step < 10; ++step) { QCoreApplication::processEvents(); }
}

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    UsbPlaylistModel model;
    model.replaceFiles({"/media/usb0/one.mp4", "/media/usb0/two.mp4"});
    PlayerController controller(model);
    MediaSessionLifecycle lifecycle;
    PlaybackCoordinator transport;
    VideoPlaybackBridge bridge;
    MockMedia media;
    media.metadataObject.setProperty("resolution", QSize(2560, 1440));
    bridge.setController(&controller);
    bridge.setLifecycle(&lifecycle);
    bridge.setTransport(&transport);
    bridge.setBackend(&media);
    transport.setController(&controller);
    transport.setBackend(&media);
    lifecycle.setDesiredSource(controller.source());
    // Model the QML binding: it is updated before a queued session is recreated.
    QObject::connect(&controller, &PlayerController::sourceChanged, &lifecycle, [&]() {
        lifecycle.setDesiredSource(controller.source());
    });
    QObject::connect(&lifecycle, &MediaSessionLifecycle::sessionChanged, &media, [&]() {
        if (lifecycle.active() == true) { media.source = lifecycle.sessionSource(); }
    });
    bridge.initialize();
    drain();
    assert(media.plays == 0); // Initial selection must not autoplay.
    controller.play();
    drain();
    assert(media.plays == 0); // Retain Play while loading.
    transport.setReady(true);
    drain();
    assert(media.plays == 1);
    bridge.reportPlayback(true, true, 10000);
    bridge.reportPosition(5000);
    assert(controller.isPlaying());
    assert(controller.duration() == 10000 && controller.position() == 5000);
    assert(bridge.playbackStarted());
    controller.pause();
    assert(media.pauses == 1 && transport.pendingPlay() == false);
    controller.seekRelative(2);
    assert(media.lastSeek == 7000);

    // Hold EOS, then Play must seek to zero and request playback again.
    controller.setAutoAdvance(false);
    controller.handleEndOfMedia();
    assert(controller.isFinished());
    const int playsBeforeReplay = media.plays;
    controller.play();
    drain();
    assert(media.lastSeek == 0 && media.plays == playsBeforeReplay + 1);

    // Error policy stops audio/video and blocks Play until a fresh session.
    bridge.reportError(true, "Missing decoder");
    assert(bridge.blocked());
    assert(controller.errorText().contains("2560 x 1440"));
    assert(controller.errorText().contains("H.264 720p"));
    assert(controller.isPlaying() == false);
    const int playsBeforeError = media.plays;
    controller.play();
    drain();
    assert(media.plays == playsBeforeError);
    bridge.reportPlayback(true, true, 50000);
    assert(controller.isPlaying() == false);

    controller.next();
    assert(lifecycle.resetting() && bridge.playbackStarted() == false && bridge.blocked() == false);
    bridge.reportPosition(9999); // Old callbacks during teardown cannot update the new track.
    assert(controller.position() == 0);
    drain();
    assert(lifecycle.sessionSource() == controller.source());
    assert(media.plays == playsBeforeError + 1);
    bridge.reportError(false, "USB read failed");
    assert(controller.errorText() == "USB read failed");

    // A synchronous construction error is surfaced once resetting ends.
    media.error = 2;
    media.errorString = "Construction failed";
    controller.next();
    drain();
    assert(bridge.blocked());
    assert(controller.errorText().contains("Hardware H.264"));
    media.error = 0;

    controller.next();
    model.replaceFiles({}); // Removal wins over a pending replacement and Play request.
    drain();
    assert(lifecycle.active() == false && controller.source().isEmpty());
    assert(transport.pendingPlay() == false);

    // Instantiate the same typed property wiring used by the QML video adapter.
    registerPlayerQmlTypes();
    UsbPlaylistModel qmlModel;
    qmlModel.replaceFiles({"/media/usb0/qml.mp4"});
    PlayerController qmlController(qmlModel);
    MockMedia qmlMedia;
    QQmlEngine engine;
    engine.rootContext()->setContextProperty("testController", &qmlController);
    engine.rootContext()->setContextProperty("testBackend", &qmlMedia);
    QQmlComponent component(&engine);
    component.setData(R"QML(
import QtQml 2.15
import UsbPlayer 1.0
QtObject {
    id: view
    property MediaSessionLifecycle life: MediaSessionLifecycle {
        desiredSource: testController.source
    }
    property PlaybackCoordinator gate: PlaybackCoordinator {
        controller: testController
        backend: testBackend
        ready: view.life.active && !view.life.resetting && !view.bridge.blocked
    }
    property VideoPlaybackBridge bridge: VideoPlaybackBridge {
        controller: testController
        lifecycle: view.life
        transport: view.gate
        backend: testBackend
    }
    property Binding mediaSource: Binding {
        target: testBackend
        property: "source"
        value: view.life.sessionSource
    }
    Component.onCompleted: view.bridge.initialize()
}
)QML", QUrl());
    QObject* view = component.create();
    if (view == nullptr) { qWarning() << component.errors(); }
    assert(view != nullptr);
    drain();
    assert(qmlMedia.source == qmlController.source());
    assert(qmlMedia.plays == 0);
    qmlController.play();
    drain();
    assert(qmlMedia.plays == 1);
    qmlController.next(); // A single-file playlist still resets the media session.
    drain();
    assert(qmlMedia.plays == 2);
    delete view;
    drain(); // Context-bound callbacks must not outlive their QML-owned QObject.

    UsbNoticeState notice;
    notice.show();
    assert(notice.shown());
    notice.show();
    assert(notice.shown());
    notice.dismiss();
    assert(notice.shown() == false);
    notice.show();
    QTimer::singleShot(7000, Qt::PreciseTimer, &app, &QCoreApplication::quit);
    app.exec();
    assert(notice.shown() == false);
    return 0;
}
#include "bridge_test.moc"
