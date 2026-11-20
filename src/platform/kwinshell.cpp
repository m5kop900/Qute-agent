#include "kwinshell.h"

#include "placementscript.h"

#include <QPointer>
#include <QQuickWindow>
#include <QTimer>
#include <QVariant>

#ifdef QUTE_HAVE_DBUS
#include <QDBusConnection>
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

// Resize for a new card level or for minimize/restore. The prompt box (the card's bottom edge) stays where it is
// on screen: %OLDROOM% is the empty room that was below it until now, so the bottom edge is found the same way
// for any change. The width changes too (the minimized sprite is only as wide as the sprite); the left edge
// stays, and the window is pulled back on screen when it would leave it.
static const char kResizeScript[] = R"JS(
(function () {
    var wins = workspace.windowList();
    for (var i = 0; i < wins.length; i++) {
        var w = wins[i];
        if (String(w.caption) !== "qute-overlay")
            continue;
        var g = w.frameGeometry;
        var cardBottom = g.y + g.height - %OLDROOM%;
        var width = %WIDTH%;
        var x = g.x;
        var out = workspace.screenAt({ x: Math.round(g.x + g.width / 2), y: Math.round(cardBottom - 1) });
        if (out) {
            var area = workspace.clientArea(KWin.PlacementArea, out, workspace.currentDesktopForScreen(out));
            x = Math.max(area.x, Math.min(g.x, area.x + area.width - width));
        }
        w.frameGeometry = { x: x, y: cardBottom - %CONTENTH%, width: width,
                            height: %CONTENTH% + %DROPROOM% };
    }
})();
)JS";


// Smart chatbox positioning. %PLACEMENTJS% is kChatboxPlacementJs (src/platform/placementscript.h).
// The sprite is an INPUT: it is kept where the classic placement would put it (at the cursor) or where it is now,
// and the card goes around it. Both scripts answer through callDBus with the chosen layout.
static const char kSmartPlaceScript[] = R"JS(
(function () {
%PLACEMENTJS%
    var cur = workspace.cursorPos;
    var wins = workspace.windowList();
    for (var i = 0; i < wins.length; i++) {
        var w = wins[i];
        if (String(w.caption) !== "qute-overlay")
            continue;
        var out = workspace.screenAt(cur);
        var area;
        if (out)
            area = workspace.clientArea(KWin.PlacementArea, out, workspace.currentDesktopForScreen(out));
        else
            area = workspace.virtualScreenGeometry;
        var S = %SPRITE%;
        // Only the SPRITE is kept on screen here; the card never moves it.
        var sx = Math.round(cur.x - 20);
        var sy = Math.round(cur.y + 20 - S);
        sx = Math.max(area.x, Math.min(sx, area.x + area.width - S));
        sy = Math.max(area.y, Math.min(sy, area.y + area.height - S));
        var r = quteChoosePlacement({ sprite: { x: sx, y: sy, w: S, h: S }, cardW: %CARDW%, cardH: %CARDH%,
                                      dropRoom: %DROPROOM%, gap: %GAP%,
                                      area: { x: area.x, y: area.y, w: area.width, h: area.height } });
        w.frameGeometry = { x: r.window.x, y: r.window.y, width: r.window.w, height: r.window.h };
        w.keepAbove = true;
        w.skipTaskbar = true;
        w.skipPager = true;
        w.skipSwitcher = true;
        workspace.activeWindow = w;
        callDBus("io.github.qute.Overlay", "/Layout", "io.github.qute.Overlay", "LayoutReady", quteWire(r));
    }
})();
)JS";

static const char kSmartResizeScript[] = R"JS(
(function () {
%PLACEMENTJS%
    var wins = workspace.windowList();
    for (var i = 0; i < wins.length; i++) {
        var w = wins[i];
        if (String(w.caption) !== "qute-overlay")
            continue;
        var g = w.frameGeometry;
        var S = %SPRITE%;
        // Where the sprite is on screen right now: the window's origin plus the sprite's offset inside it.
        var sx = Math.round(g.x + %OFFX%);
        var sy = Math.round(g.y + %OFFY%);
        var out = workspace.screenAt({ x: sx + Math.floor(S / 2), y: sy + Math.floor(S / 2) });
        var area;
        if (out)
            area = workspace.clientArea(KWin.PlacementArea, out, workspace.currentDesktopForScreen(out));
        else
            area = workspace.virtualScreenGeometry;
        var r = quteChoosePlacement({ sprite: { x: sx, y: sy, w: S, h: S }, cardW: %CARDW%, cardH: %CARDH%,
                                      dropRoom: %DROPROOM%, gap: %GAP%,
                                      area: { x: area.x, y: area.y, w: area.width, h: area.height } });
        w.frameGeometry = { x: r.window.x, y: r.window.y, width: r.window.w, height: r.window.h };
        callDBus("io.github.qute.Overlay", "/Layout", "io.github.qute.Overlay", "LayoutReady", quteWire(r));
    }
})();
)JS";

void KWinShell::prepareWindow(QQuickWindow *window)
{
    m_window = window;
#ifdef QUTE_HAVE_DBUS
    // The KWin script reports the smart layout back to this object (see LayoutReady).
    QDBusConnection bus = QDBusConnection::sessionBus();
    if (bus.registerService(QStringLiteral("io.github.qute.Overlay")))
        bus.registerObject(QStringLiteral("/Layout"), this, QDBusConnection::ExportScriptableSlots);
#endif
    // No Qt::Tool here: on KWin the script below sets skipTaskbar/keepAbove instead.
    window->setFlags(Qt::Window | Qt::FramelessWindowHint);
    window->setColor(Qt::transparent);
}

// The QML card reports its size as shownHeight/shownDrop (the minimized sprite has no drop room); the older
// property names are the fallback.
static int shownInt(QQuickWindow *window, const char *primary, const char *fallback)
{
    QVariant v = window->property(primary);
    if (!v.isValid())
        v = window->property(fallback);
    return v.toInt();
}

void KWinShell::showAtCursor(QQuickWindow *window, bool atCursor)
{
    window->show();
    window->requestActivate();
    m_dropRoom = shownInt(window, "shownDrop", "dropRoom");   // what the window is sized with right now
#ifdef QUTE_HAVE_DBUS
    if (smartEnabled(window)) {
        if (!atCursor) {
            reveal(window);
            return;
        }
        m_window = window;
        QPointer<QQuickWindow> g(window);
        // KWin must have mapped the window (with its real size) before the script can move it.
        QTimer::singleShot(80, this, [this, g] {
            if (g) {
                m_revealOnLayout = true;
                dispatchSmart(true, shownInt(g.data(), "shownHeight", "contentHeight"), shownInt(g.data(), "shownDrop", "dropRoom"));
            }
        });
        QTimer::singleShot(600, this, [g] {      // safety net: the window must appear even if KWin never answers
            if (g)
                reveal(g.data());
        });
        return;
    }
#endif
    if (!atCursor) {
        reveal(window);
        return;
    }
#ifdef QUTE_HAVE_DBUS
    const int width = window->width();
    const int contentH = shownInt(window, "shownHeight", "contentHeight");
    const int dropRoom = shownInt(window, "shownDrop", "dropRoom");
    QPointer<QQuickWindow> w(window);
    // KWin must have mapped the window (with its real size) before the script can move it.
    QTimer::singleShot(80, this, [this, w, width, contentH, dropRoom] {
        if (w)
            runPlacementScript(w.data(), width, contentH, dropRoom);   // reveals the window when done
    });
    // Safety net: if KWin never answers, the window must still appear (at the compositor's spot).
    QTimer::singleShot(600, this, [w] {
        if (w)
            reveal(w.data());
    });
#else
    reveal(window);
#endif
}

void KWinShell::resizeContent(QQuickWindow *window, int contentHeight, int dropRoom)
{
#ifdef QUTE_HAVE_DBUS
    if (smartEnabled(window)) {
        m_window = window;
        m_dropRoom = dropRoom;
        dispatchSmart(false, contentHeight, dropRoom);
        return;
    }
    const int oldRoom = m_dropRoom >= 0 ? m_dropRoom : dropRoom;
    QString js = QString::fromUtf8(kResizeScript);
    js.replace(QStringLiteral("%CONTENTH%"), QString::number(contentHeight));
    js.replace(QStringLiteral("%DROPROOM%"), QString::number(dropRoom));
    js.replace(QStringLiteral("%OLDROOM%"), QString::number(oldRoom));
    // The width the QML card wants (shownWidth); the window's current width if the property is missing.
    QVariant w = window->property("shownWidth");
    js.replace(QStringLiteral("%WIDTH%"), QString::number(w.isValid() ? w.toInt() : window->width()));
    runScript(QStringLiteral("qute-resize"), js);
    m_dropRoom = dropRoom;
#else
    PlatformShell::resizeContent(window, contentHeight, dropRoom);
#endif
}

void KWinShell::runPlacementScript(QQuickWindow *window, int width, int contentHeight, int dropRoom)
{
#ifdef QUTE_HAVE_DBUS
    QString js = QString::fromUtf8(kPlaceScript);
    js.replace(QStringLiteral("%WIDTH%"), QString::number(width));
    js.replace(QStringLiteral("%CONTENTH%"), QString::number(contentHeight));
    js.replace(QStringLiteral("%DROPROOM%"), QString::number(dropRoom));
    runScript(QStringLiteral("qute-place"), js);   // blocks until KWin has run the script
#else
    Q_UNUSED(width)
    Q_UNUSED(contentHeight)
    Q_UNUSED(dropRoom)
#endif
    reveal(window);
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

// ---- Smart chatbox positioning -----------------------------------------------------------------------------
// One script at a time: the sprite's place inside the window is only known once the previous answer arrived, so a
// request that comes in meanwhile waits (the latest one wins) and then runs with fresh numbers.
void KWinShell::dispatchSmart(bool place, int contentHeight, int dropRoom)
{
#ifdef QUTE_HAVE_DBUS
    if (!m_window)
        return;
    if (m_smartBusy) {
        m_smartPending.place = m_smartPending.valid && m_smartPending.place ? true : place;
        m_smartPending.contentHeight = contentHeight;
        m_smartPending.dropRoom = dropRoom;
        m_smartPending.valid = true;
        return;
    }
    m_smartBusy = true;
    const int seq = ++m_smartSeq;
    const SmartInputs si = smartInputs(m_window, contentHeight, dropRoom);
    QString js = QString::fromUtf8(place ? kSmartPlaceScript : kSmartResizeScript);
    js.replace(QStringLiteral("%PLACEMENTJS%"), QString::fromUtf8(kChatboxPlacementJs));
    js.replace(QStringLiteral("%SPRITE%"), QString::number(si.spriteSize));
    js.replace(QStringLiteral("%CARDW%"), QString::number(si.cardW));
    js.replace(QStringLiteral("%CARDH%"), QString::number(si.cardH));
    js.replace(QStringLiteral("%DROPROOM%"), QString::number(si.dropRoom));
    js.replace(QStringLiteral("%GAP%"), QString::number(si.gap));
    js.replace(QStringLiteral("%OFFX%"), QString::number(si.spriteOffX));
    js.replace(QStringLiteral("%OFFY%"), QString::number(si.spriteOffY));
    runScript(QStringLiteral("qute-smart"), js);
    // If the script never answers (callDBus unavailable, script error) do not stay busy forever.
    QTimer::singleShot(700, this, [this, seq] {
        if (m_smartBusy && m_smartSeq == seq)
            smartFinished();
    });
#else
    Q_UNUSED(place) Q_UNUSED(contentHeight) Q_UNUSED(dropRoom)
#endif
}

void KWinShell::LayoutReady(const QString &wire)
{
    // Runs in the GUI thread (D-Bus slot of a QObject living there).
    if (m_window)
        applyLayoutWire(m_window, wire);
    smartFinished();
}

void KWinShell::smartFinished()
{
    m_smartBusy = false;
    if (m_revealOnLayout && m_window) {
        m_revealOnLayout = false;
        reveal(m_window);                     // at its final place, with its layout
    }
    if (m_smartPending.valid) {
        const PendingSmart p = m_smartPending;
        m_smartPending = PendingSmart();
        dispatchSmart(p.place, p.contentHeight, p.dropRoom);
    }
}
