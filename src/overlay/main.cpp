// qute - overlay daemon + command line client.
//   qute [toggle]   show/hide the overlay (starts the daemon if needed)   <- bind THIS to a shortcut
//   qute show|hide|quit|reload
//   qute settings   open the settings app
//   qute --daemon   start hidden (autostart)
#include "agentstate.h"
#include "appconfig.h"
#include "conversationmodel.h"
#include "opencodecatalog.h"
#include "attachmentmodel.h"
#include "composertokens.h"
#include "opencodeclient.h"
#include "overlaycontroller.h"
#include "platformshell.h"
#include "sessionstore.h"
#include "setupcli.h"
#include "singleinstance.h"
#include "spritepackmanager.h"
#include "theme.h"

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QTimer>

#if defined(Q_OS_WIN)
#include <cstdio>
#include <windows.h>

// The executable is built as a GUI app (WIN32_EXECUTABLE), so Windows gives it no console window
// when it is launched from Explorer, and no output when it is launched from cmd/PowerShell.
// Re-attach to the console of the parent process so `qute --version`, `qute toggle --help` and the
// terminal setup wizard keep working. Returns silently when there is no parent console (autostart).
// Note: cmd.exe does not wait for GUI apps, so for an interactive wizard prefer
//   start /wait qute opencode setup
// (documented in packaging/windows/README.md).
static void attachParentConsole()
{
    if (!AttachConsole(ATTACH_PARENT_PROCESS))
        return;
    // Reopen the CRT streams onto the console we just attached to.
    if (!std::freopen("CONOUT$", "w", stdout))
        return;
    std::freopen("CONOUT$", "w", stderr);
    std::freopen("CONIN$", "r", stdin);
}
#endif

static QString ipcName()
{
    QString user = qEnvironmentVariable("USER", qEnvironmentVariable("USERNAME", QStringLiteral("user")));
    return QStringLiteral("qute-overlay-") + user;
}

int main(int argc, char *argv[])
{
#if defined(Q_OS_WIN)
    attachParentConsole();
#endif

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
        if (!OverlayController::launchSettingsApp())
            qWarning("qute: could not start qute-settings");
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
    AppConfig config;
    config.enableWatching();
    // Qt Quick Controls style from the settings: "custom" (default) is the app's own Basic look,
    // "system" keeps the platform's style (Breeze on KDE, ...), "Fusion" is Qt's portable style.
    const QString style = config.controlStyle();
    if (style != QLatin1String("system"))
        QQuickStyle::setStyle(style == QLatin1String("custom") ? QStringLiteral("Basic") : style);
    Theme theme(&config);                                // overlay colors, resolved from config + system palette
    SpritePackManager packs;
    packs.load(config.packId(), config.variant());
    QObject::connect(&config, &AppConfig::changed, &packs, [&] { packs.load(config.packId(), config.variant()); });

    OpenCodeClient client(&config);
    OpenCodeCatalog catalog(&client);
    SessionStore store(&client, &config, &catalog);
    AgentState agent;
    QObject::connect(&client, &OpenCodeClient::serverEvent, &agent, &AgentState::handleEvent);
    QObject::connect(&client, &OpenCodeClient::connectedChanged, &agent, &AgentState::setConnected);
    QObject::connect(&client, &OpenCodeClient::promptSent, &agent, &AgentState::promptSent);

    QQmlApplicationEngine engine;
    QQmlContext *ctx = engine.rootContext();
    ctx->setContextProperty(QStringLiteral("config"), &config);
    ctx->setContextProperty(QStringLiteral("theme"), &theme);
    ctx->setContextProperty(QStringLiteral("packs"), &packs);
    ctx->setContextProperty(QStringLiteral("client"), &client);
    ctx->setContextProperty(QStringLiteral("agent"), &agent);
    ctx->setContextProperty(QStringLiteral("store"), &store);
    ctx->setContextProperty(QStringLiteral("conversation"), store.conversation());
    ctx->setContextProperty(QStringLiteral("catalog"), &catalog);
    AttachmentModel attachments(&config);       // the prompt box's pending files (F1)
    ctx->setContextProperty(QStringLiteral("attachments"), &attachments);
    ComposerTools composerTools;                // "@" detection and "/" filtering for the prompt box (F1.3, F1.4)
    ctx->setContextProperty(QStringLiteral("composerTools"), &composerTools);

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
