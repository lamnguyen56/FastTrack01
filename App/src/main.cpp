#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QDebug>
#include "Core/Constants.h"

int main(int argc, char *argv[])
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    // Cấu hình attribute cho rendering trên EGLFS / KMS (chỉ cần với Qt5)
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif
    QGuiApplication app(argc, argv);
    app.setApplicationName("FastTrack-VideoPlayer");
    app.setOrganizationName("Automotive-OJT");

    qInfo() << "=======================================================";
    qInfo() << "Khởi động FastTrack Automotive Video Player (Qt5/QML)";
    qInfo() << "Target Interface:" << FastTrack::DEFAULT_CAN_INTERFACE;
    qInfo() << "USB Mount Path  :" << FastTrack::DEFAULT_USB_MOUNT_POINT;
    qInfo() << "=======================================================";

    QQmlApplicationEngine engine;

    // Load main QML file
    const QUrl url(QStringLiteral("qrc:/main.qml"));
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl)
            QCoreApplication::exit(-1);
    }, Qt::QueuedConnection);

    engine.load(url);

    return app.exec();
}
