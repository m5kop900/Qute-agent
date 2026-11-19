// qute-settings - separate settings app. Edits the same config.json the overlay watches,
// so changes apply live in the running overlay after "Save".
#include "appconfig.h"
#include "spritepackmanager.h"
#include "theme.h"

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    QGuiApplication::setApplicationName(QStringLiteral("qute"));        // MUST match the overlay (shared config path)
    QGuiApplication::setApplicationVersion(QStringLiteral(QUTE_VERSION));
    QGuiApplication::setDesktopFileName(QStringLiteral("qute-settings"));

    // The overlay uses Basic; the settings app maps the shared Theme onto the control palette, so the
    // Basic style is what makes that palette deterministic on every desktop (F7.1).
    QQuickStyle::setStyle(QStringLiteral("Basic"));

    AppConfig config;                    // no enableWatching(): this app is the editor
    SpritePackManager packs;
    packs.load(config.packId(), config.variant());
    Theme theme(&config);

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("config"), &config);
    engine.rootContext()->setContextProperty(QStringLiteral("theme"), &theme);
    engine.rootContext()->setContextProperty(QStringLiteral("packs"), &packs);
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed, &app, [] { QCoreApplication::exit(1); },
                     Qt::QueuedConnection);
    engine.loadFromModule("Qute.Settings", "Main");
    return app.exec();
}
