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

bool PlatformShell::smartEnabled(QQuickWindow *window)
{
    return window && window->property("smartPlacement").toBool();
}

PlatformShell::SmartInputs PlatformShell::smartInputs(QQuickWindow *window, int contentHeight, int dropRoom)
{
    SmartInputs in;
    in.spriteSize = window->property("smartSpriteSize").toInt();
    in.gap = window->property("smartGap").toInt();
    in.spriteOffX = window->property("spriteOffX").toInt();
    in.spriteOffY = window->property("spriteOffY").toInt();
    if (window->property("windowFolded").toBool()) {
        in.cardW = in.cardH = in.dropRoom = 0;
    } else {
        in.cardW = window->property("smartCardW").toInt();
        in.cardH = qMax(0, contentHeight - window->property("contentTop").toInt());
        in.dropRoom = dropRoom;
    }
    return in;
}

void PlatformShell::applyLayout(QQuickWindow *window, const ChatboxPlacement::Result &r)
{
    if (!window)
        return;
    window->setProperty("layoutDir", QString::fromLatin1(ChatboxPlacement::dirName(r.dir)));
    window->setProperty("layoutSpriteX", r.spriteX);
    window->setProperty("layoutSpriteY", r.spriteY);
    window->setProperty("layoutCardX", r.cardX);
    window->setProperty("layoutCardY", r.cardY);
    window->setProperty("layoutCardW", r.card.w);
    window->setProperty("layoutCardH", r.card.h);
    window->setProperty("layoutWinW", r.window.w);
    window->setProperty("layoutWinH", r.window.h);
    window->setProperty("layoutAnchorBottom", r.cardAnchoredBottom);
    window->setProperty("layoutValid", r.valid);
    window->setProperty("layoutSeq", window->property("layoutSeq").toInt() + 1);   // last: the layout is complete
}

bool PlatformShell::applyLayoutWire(QQuickWindow *window, const QString &wire)
{
    // dir,align,wx,wy,ww,wh,sx,sy,cx,cy,cw,ch,anchorBottom,valid  (ChatboxPlacement::toWire)
    const QStringList f = wire.split(QLatin1Char(','));
    if (!window || f.size() != 14)
        return false;
    ChatboxPlacement::Result r;
    bool ok = true;
    auto num = [&](int i) { bool k = false; const int v = f.at(i).toInt(&k); ok = ok && k; return v; };
    const QString d = f.at(0);
    r.dir = d == QLatin1String("left") ? ChatboxPlacement::Dir::Left
          : d == QLatin1String("above") ? ChatboxPlacement::Dir::Above
          : d == QLatin1String("below") ? ChatboxPlacement::Dir::Below : ChatboxPlacement::Dir::Right;
    r.window = ChatboxPlacement::Rect{num(2), num(3), num(4), num(5)};
    r.spriteX = num(6);
    r.spriteY = num(7);
    r.cardX = num(8);
    r.cardY = num(9);
    r.card.w = num(10);
    r.card.h = num(11);
    r.cardAnchoredBottom = num(12) != 0;
    r.valid = num(13) != 0;
    if (!ok || r.window.w <= 0 || r.window.h <= 0)
        return false;
    applyLayout(window, r);
    return true;
}
