#include "platformshell.h"

#include "genericshell.h"
#include "gnomeshell.h"
#include "kwinshell.h"
#if defined(Q_OS_MACOS)
#include "macshell.h"
#endif

#include <QGuiApplication>
#include <QQuickWindow>
#include <QtGlobal>

#if defined(QUTE_HAVE_DBUS)
#include <QDBusConnection>
#include <QDBusConnectionInterface>

// KWin can run WITHOUT Plasma (e.g. LXQt + KWin), where XDG_CURRENT_DESKTOP is not "KDE".
// So detect KWin by its D-Bus service, not by the desktop name.
static bool kwinIsRunning()
{
    QDBusConnectionInterface *iface = QDBusConnection::sessionBus().interface();
    return iface && iface->isServiceRegistered(QStringLiteral("org.kde.KWin")).value();
}

// GNOME Shell owns this name; the actual placement comes from our extension (io.github.qute.Agent).
// Without the extension GnomeShell falls back to GenericShell, which is correct on X11.
static bool gnomeShellIsRunning()
{
    QDBusConnectionInterface *iface = QDBusConnection::sessionBus().interface();
    return iface && iface->isServiceRegistered(QStringLiteral("org.gnome.Shell")).value();
}
#endif

PlatformShell *PlatformShell::create(QObject *parent)
{
#if defined(Q_OS_MACOS)
    return new MacShell(parent);
#endif
#if defined(QUTE_HAVE_DBUS)
    if (kwinIsRunning())
        return new KWinShell(parent);
    if (gnomeShellIsRunning())
        return new GnomeShell(parent);
#endif
    // Windows, X11 desktops with another WM (XFCE, ...), and anything unknown.
    return new GenericShell(parent);
}

void PlatformShell::reveal(QQuickWindow *window)
{
    if (window)
        window->setProperty("placed", true);
}

void PlatformShell::resizeContent(QQuickWindow *window, int contentHeight, int dropRoom)
{
    // Default: resize in place and keep the card's bottom edge fixed by moving the top edge.
    const QRect g = window->geometry();
    const int cardBottom = g.y() + g.height() - dropRoom;
    window->setGeometry(g.x(), cardBottom - contentHeight, g.width(), contentHeight + dropRoom);
}
