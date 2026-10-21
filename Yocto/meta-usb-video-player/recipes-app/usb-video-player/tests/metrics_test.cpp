// Check responsive metrics using the same QML resource embedded in the player.
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cmath>
#include <QCoreApplication>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QVariant>
#include <QDebug>

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    QQmlEngine engine;
    QQmlComponent component(&engine, QUrl("qrc:/ui/UiMetrics.qml"));
    if (component.isError()) {
        qWarning() << component.errors();
    }
    QObject* metrics = component.create();
    assert(metrics != nullptr);
    const auto resize = [&](int width, int height) {
        metrics->setProperty("viewportWidth", width);
        metrics->setProperty("viewportHeight", height);
        assert(metrics->property("portrait").toBool() == (height > width));
        const double header = metrics->property("headerHeight").toDouble();
        const double footer = metrics->property("footerHeight").toDouble();
        assert(header > 0 && footer > 0);
        assert(header + footer < height * 0.4); // Keep room for media and playlist.
    };
    resize(1920, 1080);
    const double baseline = metrics->property("scale").toDouble();
    assert(std::abs(baseline - 1.0) < 0.001);
    assert(metrics->property("playlistWidthFraction").toDouble() < 0.17);
    assert(metrics->property("portraitVideoFraction").toDouble() >= 0.75);
    resize(3840, 2160);
    assert(std::abs(metrics->property("scale").toDouble() - 2 * baseline) < 0.001);
    for (const auto& size : {QPair<int, int>(480, 320), {800, 480}, {1280, 720},
                             {1920, 1080}, {2560, 720}, {600, 1024}, {1080, 1920}, {480, 800}}) {
        resize(size.first, size.second);
    }
    delete metrics;
    return 0;
}
