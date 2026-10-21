#include <QGuiApplication>
#include "PlayerApplication.h"
#include "Ui/QmlCache.h"

/** @brief Initialize Qt and delegate composition to the application module. */
int main(int argc, char* argv[]) {
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif
    QGuiApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("USB-VideoPlayer"));
    app.setOrganizationName(QStringLiteral("Automotive-OJT"));

    // Configure the cache before PlayerApplication constructs its QML engine.
    QmlCache::configure();

    PlayerApplication application;
    return application.run();
}
