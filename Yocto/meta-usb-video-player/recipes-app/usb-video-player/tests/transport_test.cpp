// Exercise the actual QML transport coordinator without a display or decoder.
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <QCoreApplication>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QVariant>
#include <QDebug>

class MockTransport : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString source MEMBER source NOTIFY sourceChanged)
public:
    QString source = "file:///one.mp4";
    int plays = 0;
    int completions = 0;
    Q_INVOKABLE void play() { ++plays; }
    Q_INVOKABLE void handleEndOfMedia() { ++completions; }
signals:
    void sourceChanged();
};

static void drain() {
    for (int step = 0; step < 10; ++step) {
        QCoreApplication::processEvents();
    }
}

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    QQmlEngine engine;
    QQmlComponent component(&engine, QUrl("qrc:/ui/PlaybackCoordinator.qml"));
    if (component.isError()) {
        qWarning() << component.errors();
    }
    QObject* gate = component.create();
    assert(gate != nullptr);
    MockTransport backend;
    MockTransport controller;
    gate->setProperty("backend", QVariant::fromValue(static_cast<QObject*>(&backend)));
    gate->setProperty("controller", QVariant::fromValue(static_cast<QObject*>(&controller)));
    assert(QMetaObject::invokeMethod(gate, "requestPlay"));
    drain();
    assert(backend.plays == 0); // Play is retained while the new media is loading.
    gate->setProperty("ready", true);
    assert(backend.plays == 0); // Never call back into the backend's status handler.
    drain();
    assert(backend.plays == 1);
    assert(QMetaObject::invokeMethod(gate, "reportPlaying", Q_ARG(QVariant, QVariant(true))));
    assert(gate->property("pendingPlay").toBool() == false);

    gate->setProperty("atEnd", true);
    assert(controller.completions == 0);
    drain();
    assert(controller.completions == 1); // EOS policy runs outside the old callback.
    gate->setProperty("atEnd", false);
    gate->setProperty("ready", false);
    backend.source = controller.source = "file:///two.mp4";
    assert(QMetaObject::invokeMethod(gate, "cancel"));
    assert(QMetaObject::invokeMethod(gate, "requestPlay"));
    drain();
    assert(backend.plays == 1);
    gate->setProperty("ready", true);
    drain();
    assert(backend.plays == 2); // Automatic next starts once its new source is ready.

    assert(QMetaObject::invokeMethod(gate, "requestPlay"));
    assert(QMetaObject::invokeMethod(gate, "cancel"));
    drain();
    assert(backend.plays == 2); // Pause/Stop cancels an already queued Play.
    assert(QMetaObject::invokeMethod(gate, "requestPlay"));
    controller.source = "file:///three.mp4";
    drain();
    assert(backend.plays == 2); // A stale source cannot start after selection changes.
    backend.source = controller.source;
    assert(QMetaObject::invokeMethod(gate, "requestPlay"));
    assert(QMetaObject::invokeMethod(gate, "requestPlay"));
    drain();
    assert(backend.plays == 3); // Only the latest queued request is acted upon.
    gate->setProperty("atEnd", true);
    assert(QMetaObject::invokeMethod(gate, "cancel"));
    drain();
    assert(controller.completions == 1); // Manual Stop invalidates pending EOS work.
    delete gate;
    return 0;
}

#include "transport_test.moc"
