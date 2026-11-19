#pragma once
#include <QAbstractListModel>
#include <QImage>
#include <QVariantList>

class AppConfig;

// Files and images waiting to be sent with the next prompt (F1.1, F1.2, F1.3).
//
// Each row is sent as one entry of the prompt's `files` array: {uri, name}. The server accepts two forms
// (checked against opencode 2 with curl):
//   - "file:///absolute/path"  -> the server reads the file itself (source.type "uri")
//   - "data:<mime>;base64,..." -> inline content, used for pasted images (source.type "inline")
// The list is pure data + small Qt helpers; nothing here talks to the network.
class AttachmentModel : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)
public:
    enum Roles {
        UriRole = Qt::UserRole + 1,
        NameRole,
        MimeRole,
        SizeRole,
        DataUrlRole,
        SourceTypeRole,
        IsImageRole,
    };

    static constexpr int kMaxAttachments = 20;

    explicit AttachmentModel(AppConfig *config, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    int count() const { return m_items.size(); }

    // Each returns "" on success, otherwise a message to show the user (nothing is added then).
    Q_INVOKABLE QString addFile(const QString &urlOrPath);                 // drag & drop, in-app file picker
    // The in-app file picker (F1.1): the folder's sub-folders and the files Qute can send, as list items
    // {key: absolute path, label, sub, tag, kind: "dir" | "file"}. Hidden entries and unsupported types are left out.
    // A ".." entry leads to the parent folder. Unreadable folders give an empty list plus a "none" placeholder.
    Q_INVOKABLE QVariantList listDirectory(const QString &dir) const;
    Q_INVOKABLE QString defaultDirectory() const;                          // the working folder, or home
    Q_INVOKABLE QString addMention(const QString &relativePath, const QString &baseDir);   // "@" picker
    Q_INVOKABLE QString pasteClipboardImage();                             // Ctrl+V with an image
    QString attachImage(const QImage &image, const QString &name = QString());

    Q_INVOKABLE bool clipboardHasImage() const;
    Q_INVOKABLE bool clipboardHasText() const;
    Q_INVOKABLE void remove(int row);
    Q_INVOKABLE void clear();

    // The `files` array of the prompt body: [{uri, name}, ...].
    Q_INVOKABLE QVariantList promptFiles() const;
    // Agent references for the prompt body: [{name}] (verified against opencode 2).
    Q_INVOKABLE QVariantList agentRefs(const QStringList &names) const;

    // Mime types that can be attached. Text-like files, the common image formats, PDF and a few
    // structured-text formats. Anything else is refused with a visible message.
    static bool isSupportedMime(const QString &mime);

signals:
    void countChanged();

private:
    QString addLocalFile(const QString &path, const QString &displayName);
    qint64 sizeLimitBytes() const;

    struct Item {
        QString uri;
        QString name;
        QString mime;
        qint64 size = -1;
        QString sourceType;   // "uri" or "inline"
    };
    AppConfig *m_config;
    QList<Item> m_items;
};
