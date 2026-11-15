// qute - overlay daemon + command line client.
//   qute [toggle]   show/hide the overlay (starts the daemon if needed)   <- bind THIS to a shortcut
//   qute show|hide|quit|reload
//   qute settings   open the settings app
//   qute --daemon   start hidden (autostart)
#include "agentstate.h"
#include "appconfig.h"
#include "opencodeclient.h"
#include "overlaycontroller.h"
#include "platformshell.h"
#include "setupcli.h"
#include "singleinstance.h"
#include "spritepackmanager.h"

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QGuiApplication>
#include <QProcess>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QTimer>

static QString ipcName()
{
    QString user = qEnvironmentVariable("USER", qEnvironmentVariable("USERNAME", QStringLiteral("user")));
    return QStringLiteral("qute-overlay-") + user;
}

int main(int argc, char *argv[])
{
    // Terminal commands (`qute opencode ...`, `qute setup`) must work without a display (SSH, TTY):
    // handle them with a plain QCoreApplication BEFORE any GUI object exists.
    if (argc >= 2) {
        const QString first = QString::fromLocal8Bit(argv[1]);
        if (first == QLatin1String("opencode") || first == QLatin1String("setup")) {
            QCoreApplication cli(argc, argv);
            QCoreApplication::setApplicationName(QStringLiteral("qute"));   // same config dir as the GUI apps
            QStringList args = cli.arguments();
            args.removeFirst();                                             // program name
            return SetupCli::run(args);
        }
    }

    QQuickWindow::setDefaultAlphaBuffer(true);          // must happen before any window exists (transparency)
    QGuiApplication app(argc, argv);
    QGuiApplication::setApplicationName(QStringLiteral("qute"));        // shared with qute-settings (config path!)
    QGuiApplication::setApplicationVersion(QStringLiteral(QUTE_VERSION));
    QGuiApplication::setDesktopFileName(QStringLiteral("qute"));
    app.setQuitOnLastWindowClosed(false);

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("Qute Agent - desktop sprite assistant for opencode"));
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument(QStringLiteral("command"),
                                 QStringLiteral("toggle (default) | show | hide | quit | reload | settings | opencode ... | setup"));
    const QCommandLineOption daemonOpt(QStringLiteral("daemon"), QStringLiteral("Start in the background without showing the window."));
    parser.addOption(daemonOpt);
    parser.process(app);

    const QString cmd = parser.positionalArguments().value(0, QStringLiteral("toggle"));

    if (cmd == QLatin1String("settings")) {
        const QString local = QCoreApplication::applicationDirPath() + QStringLiteral("/qute-settings");
        if (!QProcess::startDetached(local, {}))
            QProcess::startDetached(QStringLiteral("qute-settings"), {});
        return 0;
    }

    // Already running? Forward the command and exit.
    if (parser.isSet(daemonOpt)) {
        if (SingleInstance::send(ipcName(), QStringLiteral("ping")))
            return 0;
    } else if (SingleInstance::send(ipcName(), cmd)) {
        return 0;
    }
    if (cmd == QLatin1String("quit") || cmd == QLatin1String("hide") || cmd == QLatin1String("reload"))
        return 0;                                        // nothing running, nothing to do

    // ---- we are the daemon from here on ----
    QQuickStyle::setStyle(QStringLiteral("Basic"));      // fully custom look, independent of the system style

    AppConfig config;
    config.enableWatching();
    SpritePackManager packs;
    packs.load(config.packId(), config.variant());
    QObject::connect(&config, &AppConfig::changed, &packs, [&] { packs.load(config.packId(), config.variant()); });

    OpenCodeClient client(&config);
    AgentState agent;
    QObject::connect(&client, &OpenCodeClient::serverEvent, &agent, &AgentState::handleEvent);
    QObject::connect(&client, &OpenCodeClient::connectedChanged, &agent, &AgentState::setConnected);
    QObject::connect(&client, &OpenCodeClient::promptSent, &agent, &AgentState::promptSent);

    QQmlApplicationEngine engine;
    QQmlContext *ctx = engine.rootContext();
    ctx->setContextProperty(QStringLiteral("config"), &config);
    ctx->setContextProperty(QStringLiteral("packs"), &packs);
    ctx->setContextProperty(QStringLiteral("client"), &client);
    ctx->setContextProperty(QStringLiteral("agent"), &agent);

    PlatformShell *shell = PlatformShell::create(&app);
    auto *controller = new OverlayController(shell, &config, &app);   // window is attached after load
    ctx->setContextProperty(QStringLiteral("platformName"), shell->name());
    ctx->setContextProperty(QStringLiteral("controller"), controller);

    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated, &app, [&](QObject *obj, const QUrl &) {
        if (!obj)
            QCoreApplication::exit(1);
    }, Qt::QueuedConnection);
    engine.loadFromModule("Qute.Overlay", "Main");
    if (engine.rootObjects().isEmpty())
        return 1;
    auto *window = qobject_cast<QQuickWindow *>(engine.rootObjects().first());
    if (!window)
        return 1;

    shell->prepareWindow(window);
    controller->setWindow(window);

    SingleInstance ipc(ipcName());
    if (!ipc.listen())
        qWarning("qute: could not open the IPC socket; shortcuts will not reach this instance");
    QObject::connect(&ipc, &SingleInstance::commandReceived, &app, [&](const QString &c) {
        if (c == QLatin1String("toggle")) controller->toggle();
        else if (c == QLatin1String("show")) controller->show();
        else if (c == QLatin1String("hide")) controller->hide();
        else if (c == QLatin1String("reload")) config.load();
        else if (c == QLatin1String("quit")) controller->quit();
    });

    client.start();
    if (!parser.isSet(daemonOpt))
        QTimer::singleShot(0, controller, &OverlayController::show);

    return app.exec();
}
