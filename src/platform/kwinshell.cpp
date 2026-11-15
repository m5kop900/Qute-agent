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
//   workspace.cursorPos, workspace.windowList(), workspace.clientArea(KWin.PlacementArea, window),
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
        var area = workspace.clientArea(KWin.PlacementArea, w);
        var maxH = Math.max(%MAXH%, g.height);
        var x = Math.round(cur.x - 20);
        var y = Math.round(cur.y - 20);
        x = Math.max(area.x, Math.min(x, area.x + area.width - g.width));
        y = Math.max(area.y, Math.min(y, area.y + area.height - maxH));
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
    const int maxH = qMax(window->property("maxHeight").toInt(), window->height());
    // KWin must have mapped the window (with its real size) before the script can move it.
    QTimer::singleShot(80, this, [this, maxH, atCursor] {
        if (atCursor)
            runPlacementScript(maxH);
    });
#else
    Q_UNUSED(atCursor)
#endif
}

void KWinShell::runPlacementScript(int maxHeight)
{
#ifdef QUTE_HAVE_DBUS
    QString js = QString::fromUtf8(kPlaceScript);
    js.replace(QStringLiteral("%MAXH%"), QString::number(maxHeight));

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
    Q_UNUSED(maxHeight)
#endif
}
