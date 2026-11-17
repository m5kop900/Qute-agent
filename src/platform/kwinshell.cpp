#include "kwinshell.h"

#include <QQuickWindow>
#include <QTimer>

#ifdef QUTE_HAVE_DBUS
#include <QDBusInterface>
#include <QDBusMessage>
#include <QDBusReply>
#include <QDir>
#include <QFile>
#include <QStandardPaths>
#endif

// KWin 6 scripting API (verified against develop.kde.org/docs/plasma/kwin/api):
//   workspace.cursorPos, workspace.windowList(), workspace.screenAt(point) -> KWin::Output*,
//   workspace.clientArea(KWin.PlacementArea, output, desktop) -> QRect (multi-screen aware),
//   window.frameGeometry (must be assigned as a COMPLETE {x,y,width,height} object), window.keepAbove ...
// If placement misbehaves, debug with:  journalctl -f QT_CATEGORY=kwin_scripting  (or print() output)
//
// The card's bottom edge is the prompt box. %CONTENTH% is the card height (window top edge = card top
// edge, so a transparent strip can never limit dragging the overlay up) and %DROPROOM% is empty room
// below it for the menu strip dropdowns.
static const char kPlaceScript[] = R"JS(
(function () {
    var cur = workspace.cursorPos;
    var wins = workspace.windowList();
    for (var i = 0; i < wins.length; i++) {
        var w = wins[i];
        if (String(w.caption) !== "qute-overlay")
            continue;

        // Multi-monitor: anchor to the screen that actually holds the cursor, then clamp
        // the window to that screen's working area. Resolving the area from the window
        // (the old approach) snaps it back to whatever screen it currently sits on, so a
        // toggle while the cursor is on another monitor would fail to follow the cursor.
        var out = workspace.screenAt(cur);
        var area;
        if (out)
            area = workspace.clientArea(KWin.PlacementArea, out, workspace.currentDesktopForScreen(out));
        else
            area = workspace.virtualScreenGeometry;   // cursor outside every output

        var ch = %CONTENTH%;
        var x = Math.round(cur.x - 20);
        // The prompt box (the card's bottom edge) sits just below the cursor.
        var y = Math.round(cur.y + 20 - ch);
        x = Math.max(area.x, Math.min(x, area.x + area.width - %WIDTH%));
        // Clamp on the card only: the room below it is invisible and may hang over the screen edge.
        y = Math.max(area.y, Math.min(y, area.y + area.height - ch));
        w.frameGeometry = { x: x, y: y, width: %WIDTH%, height: ch + %DROPROOM% };
        w.keepAbove = true;
        w.skipTaskbar = true;
        w.skipPager = true;
        w.skipSwitcher = true;
        workspace.activeWindow = w;
    }
})();
)JS";

// Resize for a new card level: the prompt box stays where it is on screen and the card grows upward.
static const char kResizeScript[] = R"JS(
(function () {
    var wins = workspace.windowList();
    for (var i = 0; i < wins.length; i++) {
        var w = wins[i];
        if (String(w.caption) !== "qute-overlay")
            continue;
        var g = w.frameGeometry;
        var cardBottom = g.y + g.height - %DROPROOM%;
        w.frameGeometry = { x: g.x, y: cardBottom - %CONTENTH%, width: g.width,
                            height: %CONTENTH% + %DROPROOM% };
    }
})();
)JS";

void KWinShell::prepareWindow(QQuickWindow *window)
{
    // No Qt::Tool here: on KWin the script below sets skipTaskbar/keepAbove instead.
    window->setFlags(Qt::Window | Qt::FramelessWindowHint);
    window->setColor(Qt::transparent);
}

void KWinShell::showAtCursor(QQuickWindow *window, bool atCursor)
{
    window->show();
    window->requestActivate();
#ifdef QUTE_HAVE_DBUS
    const int width = window->width();
    const int contentH = window->property("contentHeight").toInt();
    const int dropRoom = window->property("dropRoom").toInt();
    // KWin must have mapped the window (with its real size) before the script can move it.
    QTimer::singleShot(80, this, [this, width, contentH, dropRoom, atCursor] {
        if (atCursor)
            runPlacementScript(width, contentH, dropRoom);
    });
#else
    Q_UNUSED(atCursor)
#endif
}

void KWinShell::resizeContent(QQuickWindow *window, int contentHeight, int dropRoom)
{
#ifdef QUTE_HAVE_DBUS
    Q_UNUSED(window)
    QString js = QString::fromUtf8(kResizeScript);
    js.replace(QStringLiteral("%CONTENTH%"), QString::number(contentHeight));
    js.replace(QStringLiteral("%DROPROOM%"), QString::number(dropRoom));
    runScript(QStringLiteral("qute-resize"), js);
#else
    PlatformShell::resizeContent(window, contentHeight, dropRoom);
#endif
}

void KWinShell::runPlacementScript(int width, int contentHeight, int dropRoom)
{
#ifdef QUTE_HAVE_DBUS
    QString js = QString::fromUtf8(kPlaceScript);
    js.replace(QStringLiteral("%WIDTH%"), QString::number(width));
    js.replace(QStringLiteral("%CONTENTH%"), QString::number(contentHeight));
    js.replace(QStringLiteral("%DROPROOM%"), QString::number(dropRoom));
    runScript(QStringLiteral("qute-place"), js);
#else
    Q_UNUSED(width)
    Q_UNUSED(contentHeight)
    Q_UNUSED(dropRoom)
#endif
}

void KWinShell::runScript(const QString &name, QString js)
{
#ifdef QUTE_HAVE_DBUS
    const QString path = QDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation))
                             .filePath(QStringLiteral("qute-kwin-%1.js").arg(name));
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return;
    f.write(js.toUtf8());
    f.close();

    QDBusInterface scripting(QStringLiteral("org.kde.KWin"), QStringLiteral("/Scripting"),
                             QStringLiteral("org.kde.kwin.Scripting"));
    if (!scripting.isValid())
        return;
    scripting.call(QStringLiteral("unloadScript"), name);                  // drop a previous run, ignore result
    const QDBusReply<int> id = scripting.call(QStringLiteral("loadScript"), path, name);
    if (!id.isValid())
        return;

    QDBusInterface script(QStringLiteral("org.kde.KWin"),
                          QStringLiteral("/Scripting/Script%1").arg(id.value()),
                          QStringLiteral("org.kde.kwin.Script"));
    const QDBusMessage reply = script.call(QStringLiteral("run"));
    if (reply.type() == QDBusMessage::ErrorMessage)                         // object path differs on some versions
        scripting.call(QStringLiteral("start"));

    QTimer::singleShot(500, this, [name] {
        QDBusInterface s(QStringLiteral("org.kde.KWin"), QStringLiteral("/Scripting"),
                         QStringLiteral("org.kde.kwin.Scripting"));
        s.call(QStringLiteral("unloadScript"), name);
    });
#else
    Q_UNUSED(name)
    Q_UNUSED(js)
#endif
}
