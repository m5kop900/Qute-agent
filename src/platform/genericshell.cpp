#include "genericshell.h"

#include <QCursor>
#include <QRect>
#include <QGuiApplication>
#include <QQuickWindow>
#include <QScreen>

void GenericShell::prepareWindow(QQuickWindow *window)
{
    window->setFlags(Qt::Window | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    window->setColor(Qt::transparent);
}

static QScreen *screenFor(const QPoint &p, QQuickWindow *window)
{
    QScreen *screen = QGuiApplication::screenAt(p);
    if (!screen)
        screen = window ? window->screen() : nullptr;
    return screen ? screen : QGuiApplication::primaryScreen();
}

void GenericShell::smartPlace(QQuickWindow *window, bool atCursor, int contentHeight, int dropRoom)
{
    using namespace ChatboxPlacement;
    const SmartInputs si = smartInputs(window, contentHeight, dropRoom);
    if (si.spriteSize <= 0)
        return;

    Input in;
    in.cardW = si.cardW;
    in.cardH = si.cardH;
    in.dropRoom = si.dropRoom;
    in.gap = si.gap;

    if (QGuiApplication::platformName().startsWith(QLatin1String("wayland"))) {
        // A plain Wayland client cannot position its window: keep the classic arrangement (card right of the
        // sprite, growing upward) and only size the window for it.
        in.sprite = Rect{0, 0, si.spriteSize, si.spriteSize};
        in.area = Rect{-100000, -100000, 200000, 200000};
        const Result r = choose(in);
        applyLayout(window, r);
        window->resize(r.window.w, r.window.h);
        return;
    }

    QScreen *screen = nullptr;
    QRect sprite;
    if (atCursor) {
        const QPoint cur = QCursor::pos();
        screen = screenFor(cur, window);
        const QRect area = screen->availableGeometry();
        // Where the classic placement puts the sprite. Only the SPRITE is kept on screen; the card never moves it.
        int x = cur.x() - 20;
        int y = cur.y() + 20 - si.spriteSize;
        x = qBound(area.left(), x, qMax(area.left(), area.right() + 1 - si.spriteSize));
        y = qBound(area.top(), y, qMax(area.top(), area.bottom() + 1 - si.spriteSize));
        sprite = QRect(x, y, si.spriteSize, si.spriteSize);
    } else {
        const QRect g = window->geometry();
        sprite = QRect(g.x() + si.spriteOffX, g.y() + si.spriteOffY, si.spriteSize, si.spriteSize);
        screen = screenFor(sprite.center(), window);
    }
    const QRect area = screen->availableGeometry();
    in.sprite = Rect{sprite.x(), sprite.y(), sprite.width(), sprite.height()};
    in.area = Rect{area.x(), area.y(), area.width(), area.height()};
    const Result r = choose(in);
    applyLayout(window, r);                         // properties first, so the next frame already has both
    window->setGeometry(r.window.x, r.window.y, r.window.w, r.window.h);
}

void GenericShell::resizeContent(QQuickWindow *window, int contentHeight, int dropRoom)
{
    if (smartEnabled(window)) {
        smartPlace(window, false, contentHeight, dropRoom);
        return;
    }
    PlatformShell::resizeContent(window, contentHeight, dropRoom);
}

void GenericShell::showAtCursor(QQuickWindow *window, bool atCursor)
{
    if (smartEnabled(window)) {
        if (atCursor)
            smartPlace(window, true, window->property("shownHeight").toInt(), window->property("shownDrop").toInt());
        window->show();
        window->raise();
        window->requestActivate();
        reveal(window);
        return;
    }
    if (atCursor) {
        const QPoint cur = QCursor::pos();
        QScreen *screen = QGuiApplication::screenAt(cur);
        if (!screen)
            screen = QGuiApplication::primaryScreen();
        const QRect area = screen->availableGeometry();
        const int contentH = window->property("contentHeight").toInt();
        const int dropRoom = window->property("dropRoom").toInt();

        int x = cur.x() - 20;
        int y = cur.y() + 20 - contentH;      // prompt box just below the cursor; the card grows upward
        x = qBound(area.left(), x, qMax(area.left(), area.right() - window->width()));
        // Clamp on the card only: the room below it is invisible and may hang over the screen edge.
        y = qBound(area.top(), y, qMax(area.top(), area.bottom() - contentH));
        window->setGeometry(x, y, window->width(), contentH + dropRoom);
    }
    window->show();
    window->raise();
    window->requestActivate();
    reveal(window);     // positioned before show(), so the content can show right away
}
