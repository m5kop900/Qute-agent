#pragma once
#include <QFileSystemWatcher>
#include <QObject>
#include <QString>
#include <QTimer>
#include <QVariantMap>

class QJsonObject;

// All user settings, persisted as flat JSON (<AppConfigLocation>/config.json, e.g. ~/.config/qute/config.json).
// Shared by the overlay (watches the file, live reload) and the settings app (edits + save()).
//
// RULE: every setting is a Q_PROPERTY declared in this class with NOTIFY changed.
// load()/save() discover them through the meta-object, so adding a setting means
// adding the property + member + default only.
class AppConfig : public QObject {
    Q_OBJECT
    Q_PROPERTY(int configVersion READ configVersion NOTIFY changed)   // schema version, see migrate()
    Q_PROPERTY(QString configPath READ configPath CONSTANT)
    // --- opencode server ---
    Q_PROPERTY(QString serverUrl READ serverUrl WRITE setServerUrl NOTIFY changed)
    Q_PROPERTY(QString serverUsername READ serverUsername WRITE setServerUsername NOTIFY changed)
    Q_PROPERTY(QString serverPassword READ serverPassword WRITE setServerPassword NOTIFY changed)
    Q_PROPERTY(bool autoStartServer READ autoStartServer WRITE setAutoStartServer NOTIFY changed)
    Q_PROPERTY(QString serverCommand READ serverCommand WRITE setServerCommand NOTIFY changed)
    Q_PROPERTY(QString workDir READ workDir WRITE setWorkDir NOTIFY changed)
    Q_PROPERTY(QString modelProvider READ modelProvider WRITE setModelProvider NOTIFY changed)
    Q_PROPERTY(QString modelId READ modelId WRITE setModelId NOTIFY changed)
    Q_PROPERTY(QString apiFlavor READ apiFlavor WRITE setApiFlavor NOTIFY changed)   // auto | v1 | v2
    Q_PROPERTY(QString lastSessionId READ lastSessionId WRITE setLastSessionId NOTIFY changed) // resumed on start (F4.2)
    // --- appearance ---
    Q_PROPERTY(QString packId READ packId WRITE setPackId NOTIFY changed)
    Q_PROPERTY(QString variant READ variant WRITE setVariant NOTIFY changed)
    Q_PROPERTY(QString themeMode READ themeMode WRITE setThemeMode NOTIFY changed)   // system | light | dark
    Q_PROPERTY(QString accentColor READ accentColor WRITE setAccentColor NOTIFY changed) // "" = system accent
    Q_PROPERTY(double scale READ scale WRITE setScale NOTIFY changed)
    Q_PROPERTY(double overlayOpacity READ overlayOpacity WRITE setOverlayOpacity NOTIFY changed) // 0.3 .. 1.0 (F7.2)
    Q_PROPERTY(int cornerRadius READ cornerRadius WRITE setCornerRadius NOTIFY changed)           // F7.2
    Q_PROPERTY(QString density READ density WRITE setDensity NOTIFY changed)                     // compact | normal | spacious
    Q_PROPERTY(int longCardHeight READ longCardHeight WRITE setLongCardHeight NOTIFY changed)     // px of the largest card level (level 2), 300..1000
    Q_PROPERTY(int overlayWidth READ overlayWidth WRITE setOverlayWidth NOTIFY changed)           // px of the card width at scale 1.0, 380..1000
    Q_PROPERTY(double spacingScale READ spacingScale NOTIFY changed)                             // read-only, from density
    Q_PROPERTY(double radiusScale READ radiusScale NOTIFY changed)                               // read-only, from cornerRadius
    Q_PROPERTY(bool animations READ animations WRITE setAnimations NOTIFY changed)               // F7.2
    Q_PROPERTY(QString fontFamily READ fontFamily WRITE setFontFamily NOTIFY changed)            // "" = system UI font
    Q_PROPERTY(QString codeFontFamily READ codeFontFamily WRITE setCodeFontFamily NOTIFY changed) // "" = system monospace
    Q_PROPERTY(QString controlStyle READ controlStyle WRITE setControlStyle NOTIFY changed)      // custom | system | Fusion
    // Per-animation frame rate chosen by the user: { "<packId>/<state>": fps (1..60) }. A missing key = the pack default.
    Q_PROPERTY(QVariantMap spriteFps READ spriteFps WRITE setSpriteFps NOTIFY changed)
    // --- behavior ---
    Q_PROPERTY(bool placeAtCursor READ placeAtCursor WRITE setPlaceAtCursor NOTIFY changed)
    Q_PROPERTY(bool hideOnFocusLoss READ hideOnFocusLoss WRITE setHideOnFocusLoss NOTIFY changed)
    Q_PROPERTY(int maxAttachmentSizeMB READ maxAttachmentSizeMB WRITE setMaxAttachmentSizeMB NOTIFY changed) // 0 = no limit (F1.1)
    Q_PROPERTY(bool autoCompactEnabled READ autoCompactEnabled WRITE setAutoCompactEnabled NOTIFY changed)     // F5
    Q_PROPERTY(int autoCompactThreshold READ autoCompactThreshold WRITE setAutoCompactThreshold NOTIFY changed) // F5, %

public:
    explicit AppConfig(QObject *parent = nullptr);

    static QString defaultPath();

    static constexpr int CurrentConfigVersion = 2;

    // Upgrades an on-disk object in place. Public so tests can check every step without writing files.
    static void migrate(QJsonObject &obj);

    Q_INVOKABLE void load();
    Q_INVOKABLE bool save();
    void enableWatching();          // overlay only: reload when the file changes on disk

    int configVersion() const { return m_configVersion; }
    QString configPath() const { return m_path; }
    QString serverUrl() const { return m_serverUrl; }
    QString serverUsername() const { return m_serverUsername; }
    QString serverPassword() const { return m_serverPassword; }
    bool autoStartServer() const { return m_autoStartServer; }
    QString serverCommand() const { return m_serverCommand; }
    QString workDir() const { return m_workDir; }
    QString modelProvider() const { return m_modelProvider; }
    QString modelId() const { return m_modelId; }
    QString apiFlavor() const { return m_apiFlavor; }
    QString lastSessionId() const { return m_lastSessionId; }
    QString packId() const { return m_packId; }
    QString variant() const { return m_variant; }
    QString themeMode() const { return m_themeMode; }
    QString accentColor() const { return m_accentColor; }
    double scale() const { return m_scale; }
    int longCardHeight() const { return m_longCardHeight; }
    int overlayWidth() const { return m_overlayWidth; }
    double overlayOpacity() const { return m_overlayOpacity; }
    int cornerRadius() const { return m_cornerRadius; }
    QString density() const { return m_density; }
    // Derived from density / cornerRadius for the QML: 1.0 is the default look.
    double spacingScale() const { return m_density == QLatin1String("compact") ? 0.8
                                       : m_density == QLatin1String("spacious") ? 1.3 : 1.0; }
    double radiusScale() const { return m_cornerRadius / 16.0; }
    bool animations() const { return m_animations; }
    QString fontFamily() const { return m_fontFamily; }
    QString codeFontFamily() const { return m_codeFontFamily; }
    QString controlStyle() const { return m_controlStyle; }
    QVariantMap spriteFps() const { return m_spriteFps; }
    bool placeAtCursor() const { return m_placeAtCursor; }
    int maxAttachmentSizeMB() const { return m_maxAttachmentSizeMB; }
    bool hideOnFocusLoss() const { return m_hideOnFocusLoss; }
    bool autoCompactEnabled() const { return m_autoCompactEnabled; }
    int autoCompactThreshold() const { return m_autoCompactThreshold; }

    void setServerUrl(const QString &v) { assign(m_serverUrl, v); }
    void setMaxAttachmentSizeMB(int v) { assign(m_maxAttachmentSizeMB, qMax(0, v)); }
    void setServerUsername(const QString &v) { assign(m_serverUsername, v); }
    void setServerPassword(const QString &v) { assign(m_serverPassword, v); }
    void setAutoStartServer(bool v) { assign(m_autoStartServer, v); }
    void setServerCommand(const QString &v) { assign(m_serverCommand, v); }
    void setWorkDir(const QString &v) { assign(m_workDir, v); }
    void setModelProvider(const QString &v) { assign(m_modelProvider, v); }
    void setModelId(const QString &v) { assign(m_modelId, v); }
    void setApiFlavor(const QString &v) { assign(m_apiFlavor, v); }
    void setLastSessionId(const QString &v) { assign(m_lastSessionId, v); }
    void setPackId(const QString &v) { assign(m_packId, v); }
    void setVariant(const QString &v) { assign(m_variant, v); }
    void setThemeMode(const QString &v) { assign(m_themeMode, v); }
    void setAccentColor(const QString &v) { assign(m_accentColor, v); }
    void setScale(double v) { assign(m_scale, qBound(0.5, v, 3.0)); }
    void setOverlayOpacity(double v) { assign(m_overlayOpacity, qBound(0.3, v, 1.0)); }
    void setCornerRadius(int v) { assign(m_cornerRadius, qBound(0, v, 40)); }
    void setLongCardHeight(int v) { assign(m_longCardHeight, qBound(300, v, 1000)); }
    void setOverlayWidth(int v) { assign(m_overlayWidth, qBound(380, v, 1000)); }
    void setDensity(const QString &v)
    {
        const QString d = (v == QLatin1String("normal") || v == QLatin1String("spacious"))
                              ? v : QStringLiteral("compact");
        assign(m_density, d);
    }
    void setAnimations(bool v) { assign(m_animations, v); }
    void setFontFamily(const QString &v) { assign(m_fontFamily, v.trimmed()); }
    void setCodeFontFamily(const QString &v) { assign(m_codeFontFamily, v.trimmed()); }
    void setControlStyle(const QString &v)
    {
        // custom = the app's own Basic look (default); system = the platform's Qt style (Breeze on KDE, ...);
        // Fusion = Qt's portable style. Anything else falls back to custom.
        const QString s = (v == QLatin1String("system") || v == QLatin1String("Fusion"))
                              ? v : QStringLiteral("custom");
        assign(m_controlStyle, s);
    }
    void setSpriteFps(const QVariantMap &v)
    {
        QVariantMap clean;                       // keep only "<pack>/<state>" keys with a usable number
        for (auto it = v.constBegin(); it != v.constEnd(); ++it) {
            bool ok = false;
            const int n = it.value().toInt(&ok);
            if (ok && it.key().contains(QLatin1Char('/')))
                clean.insert(it.key(), qBound(1, n, 60));
        }
        assign(m_spriteFps, clean);
    }
    void setPlaceAtCursor(bool v) { assign(m_placeAtCursor, v); }
    void setHideOnFocusLoss(bool v) { assign(m_hideOnFocusLoss, v); }
    void setAutoCompactEnabled(bool v) { assign(m_autoCompactEnabled, v); }
    void setAutoCompactThreshold(int v) { assign(m_autoCompactThreshold, qBound(50, v, 95)); }

signals:
    void changed();

private:
    int m_maxAttachmentSizeMB = 0;

    template <typename T> void assign(T &member, const T &value)
    {
        if (member == value)
            return;
        member = value;
        emit changed();
    }

    QString m_path;
    int m_configVersion = CurrentConfigVersion;
    QString m_serverUrl = QStringLiteral("http://127.0.0.1:4096");
    QString m_serverUsername = QStringLiteral("opencode");
    QString m_serverPassword;
    bool m_autoStartServer = true;
    QString m_serverCommand = QStringLiteral("opencode");
    QString m_workDir;
    QString m_lastSessionId;
    QString m_modelProvider;
    QString m_modelId;
    QString m_apiFlavor = QStringLiteral("auto");
    QString m_packId = QStringLiteral("qute");
    QString m_variant = QStringLiteral("ocean");
    QString m_themeMode = QStringLiteral("system");
    QString m_accentColor;
    double m_scale = 1.0;
    int m_longCardHeight = 430;
    int m_overlayWidth = 560;
    double m_overlayOpacity = 1.0;
    int m_cornerRadius = 16;
    QString m_density = QStringLiteral("compact");
    bool m_animations = true;
    QString m_fontFamily;
    QString m_codeFontFamily;
    QString m_controlStyle = QStringLiteral("custom");
    QVariantMap m_spriteFps;
    bool m_placeAtCursor = true;
    bool m_hideOnFocusLoss = false;
    bool m_autoCompactEnabled = true;
    int m_autoCompactThreshold = 80;

    QFileSystemWatcher *m_watcher = nullptr;
    QTimer m_debounce;
};
