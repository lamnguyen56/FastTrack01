// Verify the actual QML overlay state without a display or video decoder.
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <QCoreApplication>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QVariant>

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    QQmlEngine engine;
    QQmlComponent component(&engine, QUrl("qrc:/ui/PlayerOverlayState.qml"));
    QObject* state = component.create();
    assert(state != nullptr);
    assert(state->property("title").toString() == "NO USB CONNECTED");
    assert(state->property("canPlay").toBool() == false);
    state->setProperty("mounted", true);
    assert(state->property("title").toString() == "NO MEDIA");
    state->setProperty("mediaCount", 2);
    assert(state->property("title").toString() == "READY TO PLAY");
    assert(state->property("canPlay").toBool() == true);
    state->setProperty("loading", true);
    assert(state->property("title").toString() == "LOADING VIDEO");
    assert(state->property("busy").toBool() == true);
    assert(state->property("canPlay").toBool() == false);
    state->setProperty("playbackStarted", true);
    assert(state->property("visible").toBool() == false);
    state->setProperty("loading", false);
    assert(state->property("visible").toBool() == false); // Pause/end keep the frame clear of prompts.
    state->setProperty("loading", true);
    assert(state->property("visible").toBool() == false); // Buffering an active video does not obscure it.
    state->setProperty("playbackStarted", false);
    assert(state->property("title").toString() == "LOADING VIDEO"); // Next selection.
    state->setProperty("errorText", "Decoder failed");
    assert(state->property("title").toString() == "PLAYBACK ERROR");
    assert(state->property("detail").toString() == "Decoder failed");
    state->setProperty("mounted", false);
    assert(state->property("title").toString() == "NO USB CONNECTED");
    assert(state->property("busy").toBool() == false);
    assert(state->property("canPlay").toBool() == false);
    assert(state->property("canPlay").toBool() == false);
    QQmlComponent policyComponent(&engine, QUrl("qrc:/ui/MediaSupportPolicy.qml"));
    QObject* policy = policyComponent.create();
    assert(policy != nullptr);
    QVariant message;
    assert(QMetaObject::invokeMethod(policy, "message", Q_RETURN_ARG(QVariant, message),
        Q_ARG(QVariant, QVariant(true)), Q_ARG(QVariant, QVariant(2560)),
        Q_ARG(QVariant, QVariant(1440)), Q_ARG(QVariant, QVariant("Decoder unavailable"))));
    assert(message.toString().contains("2560 x 1440"));
    assert(message.toString().contains("H.264 720p"));
    assert(QMetaObject::invokeMethod(policy, "message", Q_RETURN_ARG(QVariant, message),
        Q_ARG(QVariant, QVariant(false)), Q_ARG(QVariant, QVariant(0)),
        Q_ARG(QVariant, QVariant(0)), Q_ARG(QVariant, QVariant("USB read failed"))));
    assert(message.toString() == "USB read failed");
    delete policy;
    delete state;
    return 0;
}
