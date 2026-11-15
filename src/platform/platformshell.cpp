#include "platformshell.h"

#include "genericshell.h"
#include "kwinshell.h"
#if defined(Q_OS_MACOS)
#include "macshell.h"
#endif

#include <QGuiApplication>
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
#endif

PlatformShell *PlatformShell::create(QObject *parent)
{
#if defined(Q_OS_MACOS)
    return new MacShell(parent);
#endif
#if defined(QUTE_HAVE_DBUS)
    if (kwinIsRunning())
        return new KWinShell(parent);
#endif
    // Windows, X11 desktops with another WM (XFCE, ...), and anything unknown.
    // TODO(port): GNOME Wayland needs a GNOME Shell extension (see docs/PLATFORMS.md).
    return new GenericShell(parent);
}
