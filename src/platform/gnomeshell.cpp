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
#include <QDBusPendingReply>
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

void GnomeShell::dispatchSmart(QQuickWindow *window, bool place, int contentHeight, int dropRoom)
{
#if defined(QUTE_HAVE_DBUS)
    if (m_smartBusy) {
        m_smartPending.place = m_smartPending.valid && m_smartPending.place ? true : place;
        m_smartPending.window = window;
        m_smartPending.contentHeight = contentHeight;
        m_smartPending.dropRoom = dropRoom;
        m_smartPending.valid = true;
        return;
    }
    m_smartBusy = true;
    const SmartInputs si = smartInputs(window, contentHeight, dropRoom);
    QDBusMessage msg = QDBusMessage::createMethodCall(QString::fromLatin1(kService), QString::fromLatin1(kPath),
                                                      QString::fromLatin1(kInterface),
                                                      place ? QStringLiteral("PlaceWindowSmart") : QStringLiteral("ResizeWindowSmart"));
    if (place)
        msg.setArguments({si.spriteSize, si.cardW, si.cardH, si.dropRoom, si.gap});
    else
        msg.setArguments({si.spriteOffX, si.spriteOffY, si.spriteSize, si.cardW, si.cardH, si.dropRoom, si.gap});
    const QDBusPendingCall call = QDBusConnection::sessionBus().asyncCall(msg, 1500);
    auto *watcher = new QDBusPendingCallWatcher(call, this);
    QPointer<QQuickWindow> guard(window);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this, watcher, guard, place](QDBusPendingCallWatcher *) {
        watcher->deleteLater();
        QDBusPendingReply<QString> reply = *watcher;
        if (guard && reply.isValid() && !reply.value().isEmpty())
            applyLayoutWire(guard.data(), reply.value());
        if (guard && place)
            reveal(guard.data());                    // the window is at its final place and layout now
        m_smartBusy = false;
        if (m_smartPending.valid) {
            const PendingSmart p = m_smartPending;
            m_smartPending = PendingSmart();
            if (p.window)
                dispatchSmart(p.window.data(), p.place, p.contentHeight, p.dropRoom);
        }
    });
#else
    Q_UNUSED(window) Q_UNUSED(place) Q_UNUSED(contentHeight) Q_UNUSED(dropRoom)
#endif
}

void GnomeShell::showAtCursor(QQuickWindow *window, bool atCursor)
{
#if defined(QUTE_HAVE_DBUS)
    if (smartEnabled(window) && atCursor && extensionAvailable()) {
        window->show();
        window->requestActivate();
        QPointer<QQuickWindow> guard(window);
        QTimer::singleShot(80, this, [this, guard] {     // Mutter must have mapped the window first
            if (guard)
                dispatchSmart(guard.data(), true, guard->property("shownHeight").toInt(),
                              guard->property("shownDrop").toInt());
        });
        QTimer::singleShot(800, this, [guard] {          // a busy or broken extension must not hide the window
            if (guard)
                reveal(guard.data());
        });
        return;
    }
#endif
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
    if (smartEnabled(window) && extensionAvailable()) {
        dispatchSmart(window, false, contentHeight, dropRoom);
        return;
    }
    if (smartEnabled(window)) {                           // no extension: the C++ path (correct on X11)
        GenericShell::resizeContent(window, contentHeight, dropRoom);
        return;
    }
    if (extensionAvailable()) {
        callAgent(QStringLiteral("ResizeWindow"), {contentHeight, dropRoom});
        return;
    }
#endif
    GenericShell::resizeContent(window, contentHeight, dropRoom);
}
