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
        const int maxH = qMax(window->property("maxHeight").toInt(), window->height());

        int x = cur.x() - 20;
        int y = cur.y() - 20;
        x = qBound(area.left(), x, qMax(area.left(), area.right() - window->width()));
        y = qBound(area.top(), y, qMax(area.top(), area.bottom() - maxH));
        window->setPosition(x, y);
    }
    window->show();
    window->raise();
    window->requestActivate();
}
