// Verify the registered C++ overlay state without a display or video decoder.
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <QCoreApplication>
#include <QQmlComponent>
#include <QQmlEngine>
#include "Ui/QmlTypes.h"
#include <QVariant>

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    registerPlayerQmlTypes();
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData("import UsbPlayer 1.0\nPlayerOverlayState {}", QUrl());
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
    delete state;
    return 0;
}
