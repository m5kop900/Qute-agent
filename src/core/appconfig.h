#pragma once
#include <QFileSystemWatcher>
#include <QObject>
#include <QString>
#include <QTimer>

// All user settings, persisted as flat JSON (<AppConfigLocation>/config.json, e.g. ~/.config/qute/config.json).
// Shared by the overlay (watches the file, live reload) and the settings app (edits + save()).
//
// RULE: every setting is a Q_PROPERTY declared in this class with NOTIFY changed.
// load()/save() discover them through the meta-object, so adding a setting means
// adding the property + member + default only.
class AppConfig : public QObject {
    Q_OBJECT
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
    // --- behavior ---
    Q_PROPERTY(bool placeAtCursor READ placeAtCursor WRITE setPlaceAtCursor NOTIFY changed)
    Q_PROPERTY(bool hideOnFocusLoss READ hideOnFocusLoss WRITE setHideOnFocusLoss NOTIFY changed)
    Q_PROPERTY(int maxAttachmentSizeMB READ maxAttachmentSizeMB WRITE setMaxAttachmentSizeMB NOTIFY changed) // 0 = no limit (F1.1)

public:
    explicit AppConfig(QObject *parent = nullptr);

    static QString defaultPath();

    Q_INVOKABLE void load();
    Q_INVOKABLE bool save();
    void enableWatching();          // overlay only: reload when the file changes on disk

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
    bool placeAtCursor() const { return m_placeAtCursor; }
    int maxAttachmentSizeMB() const { return m_maxAttachmentSizeMB; }
    bool hideOnFocusLoss() const { return m_hideOnFocusLoss; }

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
    void setPlaceAtCursor(bool v) { assign(m_placeAtCursor, v); }
    void setHideOnFocusLoss(bool v) { assign(m_hideOnFocusLoss, v); }

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
    bool m_placeAtCursor = true;
    bool m_hideOnFocusLoss = false;

    QFileSystemWatcher *m_watcher = nullptr;
    QTimer m_debounce;
};
