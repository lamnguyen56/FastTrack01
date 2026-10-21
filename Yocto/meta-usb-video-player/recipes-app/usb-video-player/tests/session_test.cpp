// Verify session teardown/recreation sequencing without a GPU or decoder.
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <QCoreApplication>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QVariant>

static void drain() {
    for (int step = 0; step < 10; ++step) {
        QCoreApplication::processEvents();
    }
}

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    QQmlEngine engine;
    QQmlComponent component(&engine, QUrl("qrc:/ui/MediaSessionLifecycle.qml"));
    QObject* session = component.create();
    assert(session != nullptr);
    session->setProperty("desiredSource", QUrl("file:///one.mp4"));
    assert(QMetaObject::invokeMethod(session, "reset"));
    assert(session->property("active").toBool() == false);
    assert(session->property("resetting").toBool() == true);
    assert(session->property("sessionSource").toUrl().isEmpty());
    drain();
    assert(session->property("active").toBool() == true);
    assert(session->property("resetting").toBool() == false);
    assert(session->property("sessionSource").toUrl() == QUrl("file:///one.mp4"));

    // Same-file replay also crosses an inactive boundary (single-file AUTO NEXT).
    assert(QMetaObject::invokeMethod(session, "reset"));
    assert(session->property("active").toBool() == false);
    assert(session->property("sessionSource").toUrl().isEmpty());
    drain();
    assert(session->property("active").toBool() == true);

    session->setProperty("desiredSource", QUrl("file:///two.mp4"));
    assert(QMetaObject::invokeMethod(session, "reset"));
    session->setProperty("desiredSource", QUrl("file:///three.mp4"));
    assert(QMetaObject::invokeMethod(session, "reset"));
    drain();
    assert(session->property("sessionSource").toUrl() == QUrl("file:///three.mp4"));
    assert(session->property("active").toBool() == true);

    // Unplug wins over any queued replacement; no previous source can reappear.
    session->setProperty("desiredSource", QUrl("file:///four.mp4"));
    assert(QMetaObject::invokeMethod(session, "reset"));
    session->setProperty("desiredSource", QUrl());
    assert(QMetaObject::invokeMethod(session, "reset"));
    drain();
    assert(session->property("active").toBool() == false);
    assert(session->property("resetting").toBool() == false);
    assert(session->property("sessionSource").toUrl().isEmpty());
    delete session;
    return 0;
}
