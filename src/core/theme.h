#pragma once
#include <QColor>
#include <QObject>
#include <QString>

class AppConfig;
class QEvent;

// Overlay theme (F7.1): the card/input/text colors the QML binds to. It follows AppConfig
// (themeMode, accentColor) live and reacts to system changes (Qt color scheme and the application
// palette). The settings app still uses the system palette and can adopt this later.
//
// The resolution rules are pure static functions so the unit tests do not need a GUI session.
class Theme : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool dark READ dark NOTIFY changed)
    Q_PROPERTY(QColor accent READ accent NOTIFY changed)
    Q_PROPERTY(QColor panel READ panel NOTIFY changed)
    Q_PROPERTY(QColor input READ input NOTIFY changed)
    Q_PROPERTY(QColor text READ text NOTIFY changed)
    Q_PROPERTY(QColor textDim READ textDim NOTIFY changed)
    Q_PROPERTY(QColor border READ border NOTIFY changed)
    Q_PROPERTY(QColor windowColor READ windowColor NOTIFY changed)
    Q_PROPERTY(QColor baseColor READ baseColor NOTIFY changed)
    Q_PROPERTY(QColor alternateBase READ alternateBase NOTIFY changed)
    Q_PROPERTY(QColor buttonColor READ buttonColor NOTIFY changed)
    Q_PROPERTY(QColor buttonText READ buttonText NOTIFY changed)
    Q_PROPERTY(QColor highlightedText READ highlightedText NOTIFY changed)
public:
    explicit Theme(AppConfig *config = nullptr, QObject *parent = nullptr);

    // Pure resolution helpers (tested in tests/tst_theme.cpp).
    // mode: "system" | "light" | "dark"; scheme: Qt::styleHints()->colorScheme();
    // paletteIsDark: fallback for Qt::ColorScheme::Unknown (old platforms).
    static bool resolveDark(const QString &mode, Qt::ColorScheme scheme, bool paletteIsDark);
    static QColor resolveAccent(const QString &configured, const QColor &systemAccent);
    static QColor contrastText(const QColor &background);

    bool dark() const { return m_dark; }
    QColor accent() const { return m_accent; }
    QColor panel() const { return m_panel; }
    QColor input() const { return m_input; }
    QColor text() const { return m_text; }
    QColor textDim() const { return m_textDim; }
    QColor border() const { return m_border; }
    QColor windowColor() const { return m_window; }
    QColor baseColor() const { return m_input; }
    QColor alternateBase() const { return m_alternateBase; }
    QColor buttonColor() const { return m_button; }
    QColor buttonText() const { return m_text; }
    QColor highlightedText() const { return m_highlightedText; }

signals:
    void changed();

protected:
    // QEvent::ApplicationPaletteChange is the supported replacement for paletteChanged() since Qt 6.9.
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void refresh();
    void connectSystem();

    AppConfig *m_config = nullptr;
    QString m_mode = QStringLiteral("system");
    bool m_dark = false;
    QColor m_accent;
    QColor m_panel;
    QColor m_input;
    QColor m_text;
    QColor m_textDim;
    QColor m_border;
    QColor m_window;
    QColor m_alternateBase;
    QColor m_button;
    QColor m_highlightedText;
};
