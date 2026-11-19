#include "gnomeshell.h"

#include <QPointer>
#include <QQuickWindow>
#include <QTimer>

#include <functional>

#if defined(QUTE_HAVE_DBUS)
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusMessage>
#include <QDBusPendingCallWatcher>
#endif

namespace {
constexpr auto kService = "io.github.qute.Agent";
constexpr auto kPath = "/io/github/qute/Agent";
constexpr auto kInterface = "io.github.qute.Agent";
} // namespace

#if defined(QUTE_HAVE_DBUS)
static bool extensionAvailable()
{
    QDBusConnectionInterface *bus = QDBusConnection::sessionBus().interface();
    return bus && bus->isServiceRegistered(QString::fromLatin1(kService)).value();
}

// Fire-and-forget by default. With a callback, it runs after the extension answered (or failed)
// without blocking the GUI thread — the extension can be slow or hung, and the overlay must never
// freeze because of it.
static void callAgent(const QString &method, const QVariantList &args,
                      const std::function<void()> &done = {})
{
    QDBusMessage msg = QDBusMessage::createMethodCall(QString::fromLatin1(kService),
                                                      QString::fromLatin1(kPath),
                                                      QString::fromLatin1(kInterface), method);
    msg.setArguments(args);
    const QDBusPendingCall pending = QDBusConnection::sessionBus().asyncCall(msg, 1000);
    if (!done)
        return;
    auto *watcher = new QDBusPendingCallWatcher(pending);
    QObject::connect(watcher, &QDBusPendingCallWatcher::finished, watcher,
                     [watcher, done](QDBusPendingCallWatcher *) {
                         watcher->deleteLater();
                         done();
                     });
}
#endif

void GnomeShell::prepareWindow(QQuickWindow *window)
{
    // Same flags as everywhere; the extension raises/sticks the window on Wayland.
    GenericShell::prepareWindow(window);
}

void GnomeShell::showAtCursor(QQuickWindow *window, bool atCursor)
{
    if (!atCursor) {
        GenericShell::showAtCursor(window, false);
        return;
    }

    window->show();
    window->requestActivate();

#if defined(QUTE_HAVE_DBUS)
    if (!extensionAvailable()) {
        // No extension: plain client behavior (correct on X11, ignored by Wayland).
        GenericShell::showAtCursor(window, atCursor);
        return;
    }

    const int width = window->width();
    const int contentH = window->property("contentHeight").toInt();
    const int dropRoom = window->property("dropRoom").toInt();
    QPointer<QQuickWindow> guard(window);
    // Mutter must have mapped the window before the extension can move it. The callback reveals the
    // content only after the placement request completed, so nothing flashes at the wrong position.
    QTimer::singleShot(80, this, [width, contentH, dropRoom, guard] {
        if (!guard)
            return;
        callAgent(QStringLiteral("PlaceWindow"), {width, contentH, dropRoom}, [guard] {
            if (guard)
                reveal(guard.data());
        });
    });
    // Safety net: a busy/broken extension must not leave the window invisible.
    QTimer::singleShot(800, this, [guard] {
        if (guard)
            reveal(guard.data());
    });
#else
    GenericShell::showAtCursor(window, atCursor);
#endif
}

void GnomeShell::resizeContent(QQuickWindow *window, int contentHeight, int dropRoom)
{
#if defined(QUTE_HAVE_DBUS)
    if (extensionAvailable()) {
        callAgent(QStringLiteral("ResizeWindow"), {contentHeight, dropRoom});
        return;
    }
#endif
    PlatformShell::resizeContent(window, contentHeight, dropRoom);
}
