#pragma once
#include <QColor>
#include <QFont>
#include <QObject>
#include <QString>

class AppConfig;
class QEvent;

// Overlay and settings theme: every color, font family and style decision the QML needs.
// It follows AppConfig (themeMode, accentColor, fontFamily, codeFontFamily) live and reacts to system
// changes (Qt color scheme, the application palette and the system font). Both executables create one
// Theme, so the overlay and the settings app always look the same.
//
// Colors: in "system" mode the surfaces and text come from the desktop palette (Breeze on KDE, GTK on
// GNOME, ...). In "light" and "dark" mode they are fixed. The status and syntax colors are always
// fixed, so an error is red everywhere, and they can be read from QML as theme.danger etc.
//
// Fonts: the UI font is the system font unless fontFamily is set. Theme writes it into
// QGuiApplication::font(), so every QML Text that does not set its own family follows it. The code
// font (codeFamily) is used explicitly by the code blocks.
//
// The resolution rules are pure static functions so the unit tests do not need a GUI session.
class Theme : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool dark READ dark NOTIFY changed)
    Q_PROPERTY(QColor accent READ accent NOTIFY changed)
    Q_PROPERTY(QColor accentAlt READ accentAlt NOTIFY changed)
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
    Q_PROPERTY(QColor danger READ danger NOTIFY changed)
    Q_PROPERTY(QColor warning READ warning NOTIFY changed)
    Q_PROPERTY(QColor success READ success NOTIFY changed)
    Q_PROPERTY(QColor codeBackground READ codeBackground NOTIFY changed)
    Q_PROPERTY(QColor syntaxString READ syntaxString NOTIFY changed)
    Q_PROPERTY(QColor syntaxNumber READ syntaxNumber NOTIFY changed)
    Q_PROPERTY(QString family READ family NOTIFY changed)
    Q_PROPERTY(QString codeFamily READ codeFamily NOTIFY changed)
public:
    explicit Theme(AppConfig *config = nullptr, QObject *parent = nullptr);

    // Pure resolution helpers (tested in tests/tst_theme.cpp).
    // mode: "system" | "light" | "dark"; scheme: Qt::styleHints()->colorScheme();
    // paletteIsDark: fallback for Qt::ColorScheme::Unknown (old platforms).
    static bool resolveDark(const QString &mode, Qt::ColorScheme scheme, bool paletteIsDark);
    static QColor resolveAccent(const QString &configured, const QColor &systemAccent);
    static QColor contrastText(const QColor &background);
    // Font family precedence: a non-empty configured family wins, otherwise the fallback (system font).
    static QString resolveFamily(const QString &configured, const QString &fallback);

    bool dark() const { return m_dark; }
    QColor accent() const { return m_accent; }
    QColor accentAlt() const { return m_accentAlt; }
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
    QColor danger() const { return m_danger; }
    QColor warning() const { return m_warning; }
    QColor success() const { return m_success; }
    QColor codeBackground() const { return m_codeBackground; }
    QColor syntaxString() const { return m_syntaxString; }
    QColor syntaxNumber() const { return m_syntaxNumber; }
    QString family() const { return m_family; }
    QString codeFamily() const { return m_codeFamily; }

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
    QColor m_accentAlt;
    QColor m_panel;
    QColor m_input;
    QColor m_text;
    QColor m_textDim;
    QColor m_border;
    QColor m_window;
    QColor m_alternateBase;
    QColor m_button;
    QColor m_highlightedText;
    QColor m_danger;
    QColor m_warning;
    QColor m_success;
    QColor m_codeBackground;
    QColor m_syntaxString;
    QColor m_syntaxNumber;
    QString m_family;
    QString m_codeFamily;
    QFont m_systemFont;     // the desktop's UI font, remembered before we ever change the app font
    QFont m_appliedFont;    // what we last put into QGuiApplication::font()
};
