#include "genericshell.h"

#include <QCursor>
#include <QGuiApplication>
#include <QQuickWindow>
#include <QScreen>

void GenericShell::prepareWindow(QQuickWindow *window)
{
    window->setFlags(Qt::Window | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    window->setColor(Qt::transparent);
}

void GenericShell::showAtCursor(QQuickWindow *window, bool atCursor)
{
    if (atCursor) {
        const QPoint cur = QCursor::pos();
        QScreen *screen = QGuiApplication::screenAt(cur);
        if (!screen)
            screen = QGuiApplication::primaryScreen();
        const QRect area = screen->availableGeometry();
        const int contentH = window->property("contentHeight").toInt();
        const int maxContentH = window->property("maxContentHeight").toInt();

        int x = cur.x() - 20;
        int y = cur.y() + 20 - contentH;      // prompt box just below the cursor; the card grows downward
        x = qBound(area.left(), x, qMax(area.left(), area.right() - window->width()));
        // Keep room for the tallest card so a reply never grows off the screen.
        y = qBound(area.top(), y, qMax(area.top(), area.bottom() - maxContentH));
        window->setPosition(x, y);
    }
    window->show();
    window->raise();
    window->requestActivate();
}
