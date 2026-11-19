#include "theme.h"
#include "appconfig.h"

#include <QEvent>
#include <QGuiApplication>
#include <QPalette>
#include <QStyleHints>

namespace {
QColor withAlpha(const QColor &c, int alpha)
{
    QColor out = c;
    out.setAlpha(alpha);
    return out;
}
} // namespace

Theme::Theme(AppConfig *config, QObject *parent) : QObject(parent), m_config(config)
{
    if (m_config)
        connect(m_config, &AppConfig::changed, this, &Theme::refresh);
    connectSystem();
    refresh();
}

void Theme::connectSystem()
{
    // Qt 6.5+: the platform reports the color scheme (KDE, GNOME, LXQt with a recent platform theme).
    if (auto *hints = QGuiApplication::styleHints())
        connect(hints, &QStyleHints::colorSchemeChanged, this, &Theme::refresh);
    // Palette changes still matter for platforms that only report Qt::ColorScheme::Unknown.
    // The event is delivered to the application object, so filter it there.
    if (qGuiApp)
        qGuiApp->installEventFilter(this);
}

bool Theme::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::ApplicationPaletteChange)
        refresh();
    return QObject::eventFilter(watched, event);
}

bool Theme::resolveDark(const QString &mode, Qt::ColorScheme scheme, bool paletteIsDark)
{
    if (mode == QLatin1String("dark"))
        return true;
    if (mode == QLatin1String("light"))
        return false;
    if (scheme == Qt::ColorScheme::Dark)
        return true;
    if (scheme == Qt::ColorScheme::Light)
        return false;
    return paletteIsDark;                    // unknown mode or unknown scheme: follow the palette
}

QColor Theme::resolveAccent(const QString &configured, const QColor &systemAccent)
{
    const QColor fromConfig(configured);
    if (fromConfig.isValid())
        return fromConfig;
    if (systemAccent.isValid())
        return systemAccent;
    return QColor(QStringLiteral("#6ea8ff"));
}

QColor Theme::contrastText(const QColor &background)
{
    const double l = 0.2126 * background.redF() + 0.7152 * background.greenF() + 0.0722 * background.blueF();
    return l > 0.55 ? QColor(QStringLiteral("#10121c")) : QColor(QStringLiteral("#ffffff"));
}

void Theme::refresh()
{
    m_mode = m_config ? m_config->themeMode() : QStringLiteral("system");
    const bool useSystem = m_mode == QLatin1String("system");
    const QPalette pal = QGuiApplication::palette();
    const Qt::ColorScheme scheme = QGuiApplication::styleHints()
                                       ? QGuiApplication::styleHints()->colorScheme()
                                       : Qt::ColorScheme::Unknown;
    const bool paletteDark = pal.color(QPalette::Window).lightness() < 128;
    m_dark = resolveDark(m_mode, scheme, paletteDark);

    const QString configuredAccent = m_config ? m_config->accentColor() : QString();
    // The fixed light/dark palettes do not inherit the desktop highlight color (same rule as the
    // previous QML Theme); only "system" does.
    const QColor systemAccent = useSystem ? pal.color(QPalette::Active, QPalette::Highlight) : QColor();
    m_accent = resolveAccent(configuredAccent, systemAccent);
    m_highlightedText = useSystem && configuredAccent.isEmpty()
                            ? pal.color(QPalette::Active, QPalette::HighlightedText)
                            : contrastText(m_accent);

    if (useSystem) {
        m_window = pal.color(QPalette::Window);
        m_text = pal.color(QPalette::WindowText);
        m_input = pal.color(QPalette::Base);
        m_alternateBase = pal.color(QPalette::AlternateBase);
        m_button = pal.color(QPalette::Button);
        m_panel = withAlpha(m_window, 0xf2);
    } else {
        m_window = m_dark ? QColor(QStringLiteral("#1a1c2b")) : QColor(QStringLiteral("#f7f8fc"));
        m_text = m_dark ? QColor(QStringLiteral("#e8eaf6")) : QColor(QStringLiteral("#1d2033"));
        m_input = m_dark ? QColor(QStringLiteral("#262a3d")) : QColor(QStringLiteral("#ffffff"));
        m_alternateBase = m_dark ? QColor(QStringLiteral("#22263a")) : QColor(QStringLiteral("#f0f2f8"));
        m_button = m_dark ? QColor(QStringLiteral("#2e3348")) : QColor(QStringLiteral("#eef1f7"));
        m_panel = m_dark ? QColor(0x1a, 0x1c, 0x2b, 0xf2) : QColor(0xf7, 0xf8, 0xfc, 0xf2);
    }
    m_textDim = withAlpha(m_text, 0.55 * 255);
    m_border = withAlpha(m_text, 0.18 * 255);
    emit changed();
}
