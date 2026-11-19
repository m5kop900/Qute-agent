#include "theme.h"
#include "appconfig.h"

#include <QEvent>
#include <QFontDatabase>
#include <QGuiApplication>
#include <QPalette>
#include <QStyleHints>
#include <QtGlobal>

namespace {
QColor withAlpha(const QColor &c, int alpha)
{
    QColor out = c;
    out.setAlpha(alpha);
    return out;
}

// Linear blend: t = 0 gives a, t = 1 gives b.
QColor mix(const QColor &a, const QColor &b, double t)
{
    return QColor::fromRgbF(a.redF() + (b.redF() - a.redF()) * t,
                            a.greenF() + (b.greenF() - a.greenF()) * t,
                            a.blueF() + (b.blueF() - a.blueF()) * t);
}
} // namespace

Theme::Theme(AppConfig *config, QObject *parent) : QObject(parent), m_config(config)
{
    m_systemFont = QGuiApplication::font();
    m_appliedFont = m_systemFont;
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
    // Palette and font changes come from the platform theme. Both are delivered to the application
    // object, so filter them there.
    if (qGuiApp)
        qGuiApp->installEventFilter(this);
}

bool Theme::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::ApplicationPaletteChange) {
        refresh();
    } else if (event->type() == QEvent::ApplicationFontChange) {
        // A font change that we did not make is the system font changing: remember it as the new base.
        if (QGuiApplication::font() != m_appliedFont)
            m_systemFont = QGuiApplication::font();
        refresh();
    }
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

QString Theme::resolveFamily(const QString &configured, const QString &fallback)
{
    const QString trimmed = configured.trimmed();
    return trimmed.isEmpty() ? fallback : trimmed;
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
    // The fixed light/dark palettes do not inherit the desktop highlight color; only "system" does.
    const QColor systemAccent = useSystem ? pal.color(QPalette::Active, QPalette::Highlight) : QColor();
    m_accent = resolveAccent(configuredAccent, systemAccent);
    m_highlightedText = useSystem && configuredAccent.isEmpty()
                            ? pal.color(QPalette::Active, QPalette::HighlightedText)
                            : contrastText(m_accent);

    // Overlay opacity (0.3 .. 1.0) is applied to the card background only; text and controls stay solid.
    const double opacity = m_config ? m_config->overlayOpacity() : 1.0;
    const int panelAlpha = qRound(255 * qBound(0.3, opacity, 1.0));

    if (useSystem) {
        m_window = pal.color(QPalette::Window);
        m_text = pal.color(QPalette::WindowText);
        m_input = pal.color(QPalette::Base);
        m_alternateBase = pal.color(QPalette::AlternateBase);
        m_button = pal.color(QPalette::Button);
        m_panel = withAlpha(m_window, panelAlpha);
    } else {
        m_window = m_dark ? QColor(QStringLiteral("#1a1c2b")) : QColor(QStringLiteral("#f7f8fc"));
        m_text = m_dark ? QColor(QStringLiteral("#e8eaf6")) : QColor(QStringLiteral("#1d2033"));
        m_input = m_dark ? QColor(QStringLiteral("#262a3d")) : QColor(QStringLiteral("#ffffff"));
        m_alternateBase = m_dark ? QColor(QStringLiteral("#22263a")) : QColor(QStringLiteral("#f0f2f8"));
        m_button = m_dark ? QColor(QStringLiteral("#2e3348")) : QColor(QStringLiteral("#eef1f7"));
        m_panel = m_dark ? QColor(0x1a, 0x1c, 0x2b, panelAlpha) : QColor(0xf7, 0xf8, 0xfc, panelAlpha);
    }
    m_textDim = withAlpha(m_text, 0.55 * 255);
    m_border = withAlpha(m_text, 0.18 * 255);
    m_codeBackground = mix(m_input, m_text, 0.07);

    // Status and syntax colors: fixed, so the same meaning looks the same on every desktop.
    m_danger = m_dark ? QColor(QStringLiteral("#ff6b6f")) : QColor(QStringLiteral("#d93a3f"));
    m_warning = m_dark ? QColor(QStringLiteral("#f0b35a")) : QColor(QStringLiteral("#b86e00"));
    m_success = m_dark ? QColor(QStringLiteral("#5fd39a")) : QColor(QStringLiteral("#1f8a57"));
    m_accentAlt = m_dark ? QColor(QStringLiteral("#ff9ccc")) : QColor(QStringLiteral("#d6457f"));
    m_syntaxString = m_dark ? QColor(QStringLiteral("#98c379")) : QColor(QStringLiteral("#3d7a22"));
    m_syntaxNumber = m_dark ? QColor(QStringLiteral("#d19a66")) : QColor(QStringLiteral("#a4621a"));

    // Fonts: the configured family, or the desktop's own font and monospace font.
    const QString configuredFamily = m_config ? m_config->fontFamily() : QString();
    const QString configuredCode = m_config ? m_config->codeFontFamily() : QString();
    m_family = resolveFamily(configuredFamily, m_systemFont.family());
    m_codeFamily = resolveFamily(configuredCode,
                                 QFontDatabase::systemFont(QFontDatabase::FixedFont).family());

    QFont appFont = m_systemFont;
    appFont.setFamily(m_family);
    m_appliedFont = appFont;
    if (appFont != QGuiApplication::font())
        QGuiApplication::setFont(appFont);   // QML Text without its own family follows this

    emit changed();
}
