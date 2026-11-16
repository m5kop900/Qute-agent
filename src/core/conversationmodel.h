#pragma once
#include <QAbstractListModel>
#include <QJsonArray>
#include <QJsonObject>
#include <QString>

// Ordered conversation items for the overlay (F0.1). Pure projection: it knows NOTHING about
// networking, QML or the sprite state - events and the projected message list are handed in.
//
// Item kinds (strings, also the contract with QML):
//   user            - a prompt typed by the user
//   assistant-text  - streamed answer text
//   reasoning       - the model's reasoning / thinking text (collapsible in QML)
//   tool            - one tool call (name + status + short input summary)
//   request         - a permission request or a question form waiting for the user
//   notice          - errors, compaction status' etc.
//
// Two sources, one shape: live events (session.text.*, session.tool.*, message.part.* ...) and the
// server's projected message list (GET .../message). Replaying a recorded event sequence must produce
// the same items as the projection of the same turn (see tests/tst_conversationmodel.cpp).
class ConversationModel : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)
public:
    enum Role {
        KindRole = Qt::UserRole + 1,
        IdRole,
        TextRole,
        NameRole,           // tool name, request action
        StatusRole,         // tool: running|ok|error ; request: pending|answered|cancelled
        RequestKindRole,    // permission|form
        DetailRole,         // short input/output summary, tooltip text
        StreamingRole,      // text is still arriving
    };

    struct Item {
        QString kind;
        QString id;
        QString text;
        QString name;
        QString status;
        QString requestKind;
        QString detail;
        bool streaming = false;
    };

    explicit ConversationModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    const QList<Item> &items() const { return m_items; }

    // Feed one bus event (V1 or V2). `properties` is the event payload (already unwrapped).
    void applyEvent(const QString &type, const QJsonObject &properties);
    // The user sent this prompt right now (before the server echoes it back).
    void applyUserPrompt(const QString &text);
    // Replace the timeline with the server's projected messages, oldest first.
    // Pending permission/form requests are preserved because the projection does not contain them.
    void applyMessages(const QJsonArray &messagesAscending);
    Q_INVOKABLE void clear();
    Q_INVOKABLE void addNotice(const QString &text);   // local feedback (slash commands, errors)

signals:
    void countChanged();

private:
    int indexOf(const QString &kind, const QString &id) const;
    int upsert(const Item &item);                  // insert or merge; returns the row
    void touch(int row);
    void setStreaming(int row, bool streaming);
    void appendNotice(const QString &text);
    static QString roleOf(const QJsonObject &message);
    static QJsonArray partsOf(const QJsonObject &message);
    static QString textOfParts(const QJsonArray &parts);
    static QString statusOfTool(const QJsonObject &part);
    static QString summarizeInput(const QJsonValue &input, const QString &fallback);
    // Full tool input/output as readable text (shown when the user opens a tool card).
    static QString toolDetailText(const QJsonValue &input, const QJsonValue &output, const QString &error = {});
    static QJsonValue toolInputOf(const QJsonObject &part);
    static QJsonValue toolOutputOf(const QJsonObject &part);

    QList<Item> m_items;
    int m_userPromptSeq = 0;
};
