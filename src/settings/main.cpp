// qute-settings - separate settings app. Edits the same config.json the overlay watches,
// so changes apply live in the running overlay after "Save".
#include "appconfig.h"
#include "overlaycontrol.h"
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

    AppConfig config;                    // no enableWatching(): this app is the editor
    // Qt Quick Controls style from the settings: "custom" (default) is the app's own Basic look,
    // "system" keeps the platform's style (Breeze on KDE, ...), "Fusion" is Qt's portable style.
    const QString style = config.controlStyle();
    if (style != QLatin1String("system"))
        QQuickStyle::setStyle(style == QLatin1String("custom") ? QStringLiteral("Basic") : style);
    SpritePackManager packs;
    packs.load(config.packId(), config.variant());
    Theme theme(&config);
    OverlayControl overlay;              // restart the running overlay after saving

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("config"), &config);
    engine.rootContext()->setContextProperty(QStringLiteral("theme"), &theme);
    engine.rootContext()->setContextProperty(QStringLiteral("packs"), &packs);
    engine.rootContext()->setContextProperty(QStringLiteral("overlay"), &overlay);
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed, &app, [] { QCoreApplication::exit(1); },
                     Qt::QueuedConnection);
    engine.loadFromModule("Qute.Settings", "Main");
    return app.exec();
}
