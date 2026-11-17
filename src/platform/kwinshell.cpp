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
static const char kPlaceScript[] = R"JS(
(function () {
    var cur = workspace.cursorPos;
    var wins = workspace.windowList();
    for (var i = 0; i < wins.length; i++) {
        var w = wins[i];
        if (String(w.caption) !== "qute-overlay")
            continue;
        var g = w.frameGeometry;

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

        var x = Math.round(cur.x - 20);
        // The card's bottom edge (the prompt box, %CONTENTH% px below the window top) sits just below the
        // cursor. The card is anchored to the window top and grows downward, so the visible top edge is
        // always the window's top edge - no invisible window space above it to limit dragging.
        var y = Math.round(cur.y + 20 - %CONTENTH%);
        x = Math.max(area.x, Math.min(x, area.x + area.width - g.width));
        // Keep room for the tallest card (%MAXCONTENTH%) so a reply never grows off the screen.
        y = Math.max(area.y, Math.min(y, area.y + area.height - %MAXCONTENTH%));
        w.frameGeometry = { x: x, y: y, width: g.width, height: g.height };
        w.keepAbove = true;
        w.skipTaskbar = true;
        w.skipPager = true;
        w.skipSwitcher = true;
        workspace.activeWindow = w;
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
    const int contentH = window->property("contentHeight").toInt();
    const int maxContentH = window->property("maxContentHeight").toInt();
    // KWin must have mapped the window (with its real size) before the script can move it.
    QTimer::singleShot(80, this, [this, contentH, maxContentH, atCursor] {
        if (atCursor)
            runPlacementScript(contentH, maxContentH);
    });
#else
    Q_UNUSED(atCursor)
#endif
}

void KWinShell::runPlacementScript(int contentHeight, int maxContentHeight)
{
#ifdef QUTE_HAVE_DBUS
    QString js = QString::fromUtf8(kPlaceScript);
    js.replace(QStringLiteral("%CONTENTH%"), QString::number(contentHeight));
    js.replace(QStringLiteral("%MAXCONTENTH%"), QString::number(maxContentHeight));

    const QString path = QDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation))
                             .filePath(QStringLiteral("qute-kwin-place.js"));
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return;
    f.write(js.toUtf8());
    f.close();

    const QString name = QStringLiteral("qute-place");
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
    Q_UNUSED(contentHeight)
    Q_UNUSED(maxContentHeight)
#endif
}
