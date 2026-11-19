#pragma once
#include <QAbstractListModel>
#include <QJsonArray>
#include <QJsonObject>
#include <QString>
#include <QStringList>

// Ordered conversation items for the overlay (F0.1). Pure projection: it knows NOTHING about
// networking, QML or the sprite state - events and the projected message list are handed in.
//
// Item kinds (strings, also the contract with QML):
//   user            - a prompt typed by the user
//   assistant-text  - streamed answer text
//   reasoning       - the model's reasoning / thinking text (collapsible in QML)
//   tool            - one tool call (name + status + short input summary)
//   request         - a permission request or a question form waiting for the user
//   compaction      - the summary the server wrote when the session was compacted (collapsible in QML)
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
        AttachmentsRole,    // user prompts: names of the files sent with them (list of strings)
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
        QStringList attachments;   // user prompts: file names sent with the prompt
        // Local rows (notices, prompts not echoed yet). They are placed by how many server rows came before them,
        // not by id: the live rows and the history rows have different ids, so an id anchor was lost on refresh.
        bool local = false;
        int serverRows = -1;     // local rows: server rows that were present when the row was added
    };

    explicit ConversationModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    const QList<Item> &items() const { return m_items; }

    // Feed one bus event (V1 or V2). `properties` is the event payload (already unwrapped).
    void applyEvent(const QString &type, const QJsonObject &properties);
    // The user sent this prompt right now (before the server echoes it back).
    void applyUserPrompt(const QString &text, const QStringList &attachmentNames = {});
    // Replace the timeline with the server's projected messages, oldest first.
    // Pending permission/form requests are preserved because the projection does not contain them.
    void applyMessages(const QJsonArray &messagesAscending);
    Q_INVOKABLE void clear();
    Q_INVOKABLE void addNotice(const QString &text);   // local feedback (slash commands, errors)
    // Removes the "Retrying…" line once the turn is over (finished or stopped).
    void clearRetryNotice();
    int countServerRows() const;
    // Text of the newest assistant answer (for /copy). Empty when there is none.
    Q_INVOKABLE QString lastAssistantText() const;

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
