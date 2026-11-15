// qute-settings - separate settings app. Edits the same config.json the overlay watches,
// so changes apply live in the running overlay after "Save".
#include "appconfig.h"
#include "spritepackmanager.h"

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    QGuiApplication::setApplicationName(QStringLiteral("qute"));        // MUST match the overlay (shared config path)
    QGuiApplication::setApplicationVersion(QStringLiteral(QUTE_VERSION));
    QGuiApplication::setDesktopFileName(QStringLiteral("qute-settings"));

    AppConfig config;                    // no enableWatching(): this app is the editor
    SpritePackManager packs;
    packs.load(config.packId(), config.variant());

    QQmlApplicationEngine engine;       // default (system) Quick Controls style on purpose
    engine.rootContext()->setContextProperty(QStringLiteral("config"), &config);
    engine.rootContext()->setContextProperty(QStringLiteral("packs"), &packs);
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed, &app, [] { QCoreApplication::exit(1); },
                     Qt::QueuedConnection);
    engine.loadFromModule("Qute.Settings", "Main");
    return app.exec();
}
