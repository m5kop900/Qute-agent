#include "conversationmodel.h"

#include <QJsonDocument>
#include <QSet>

static QString firstString(const QJsonObject &o, std::initializer_list<const char *> keys)
{
    for (const char *k : keys) {
        const QString v = o.value(QLatin1String(k)).toString();
        if (!v.isEmpty())
            return v;
    }
    return {};
}

static QString normalizedToolStatus(const QString &s)
{
    if (s == QLatin1String("pending") || s == QLatin1String("running") || s == QLatin1String("called")
        || s == QLatin1String("input") || s == QLatin1String("in_progress"))
        return QStringLiteral("running");
    if (s == QLatin1String("completed") || s == QLatin1String("success") || s == QLatin1String("succeeded")
        || s == QLatin1String("ok"))
        return QStringLiteral("ok");
    if (s == QLatin1String("failed") || s == QLatin1String("error"))
        return QStringLiteral("error");
    return s.isEmpty() ? QStringLiteral("running") : s;
}

ConversationModel::ConversationModel(QObject *parent) : QAbstractListModel(parent) {}

int ConversationModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_items.size();
}

QVariant ConversationModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_items.size())
        return {};
    const Item &it = m_items.at(index.row());
    switch (role) {
    case KindRole:        return it.kind;
    case IdRole:          return it.id;
    case TextRole:        return it.text;
    case NameRole:        return it.name;
    case StatusRole:      return it.status;
    case RequestKindRole: return it.requestKind;
    case DetailRole:      return it.detail;
    case StreamingRole:   return it.streaming;
    default:              return {};
    }
}

QHash<int, QByteArray> ConversationModel::roleNames() const
{
    return {
        {KindRole, "kind"},           {IdRole, "id"},               {TextRole, "text"},
        {NameRole, "name"},           {StatusRole, "status"},       {RequestKindRole, "requestKind"},
        {DetailRole, "detail"},       {StreamingRole, "streaming"},
    };
}

int ConversationModel::indexOf(const QString &kind, const QString &id) const
{
    for (int i = 0; i < m_items.size(); ++i)
        if (m_items.at(i).kind == kind && m_items.at(i).id == id)
            return i;
    return -1;
}

void ConversationModel::touch(int row)
{
    if (row < 0 || row >= m_items.size())
        return;
    const QModelIndex idx = index(row);
    emit dataChanged(idx, idx, {KindRole, TextRole, NameRole, StatusRole, RequestKindRole, DetailRole, StreamingRole});
}

void ConversationModel::setStreaming(int row, bool streaming)
{
    if (row < 0 || row >= m_items.size() || m_items.at(row).streaming == streaming)
        return;
    m_items[row].streaming = streaming;
    touch(row);
}

int ConversationModel::upsert(const Item &item)
{
    const int existing = indexOf(item.kind, item.id);
    if (existing >= 0) {
        Item &cur = m_items[existing];
        // Merge: an update event often carries only the id (e.g. tool.success). Never wipe a field with an empty one.
        if (!item.text.isEmpty())        cur.text = item.text;
        if (!item.name.isEmpty())        cur.name = item.name;
        if (!item.status.isEmpty())      cur.status = item.status;
        if (!item.requestKind.isEmpty()) cur.requestKind = item.requestKind;
        if (!item.detail.isEmpty())      cur.detail = item.detail;
        touch(existing);
        return existing;
    }
    const int row = m_items.size();
    beginInsertRows({}, row, row);
    m_items.append(item);
    endInsertRows();
    emit countChanged();
    return row;
}

void ConversationModel::appendNotice(const QString &text)
{
    if (text.isEmpty())
        return;
    Item it;
    it.kind = QStringLiteral("notice");
    it.id = QStringLiteral("notice:%1").arg(m_items.size());
    it.text = text;
    upsert(it);
}

QString ConversationModel::roleOf(const QJsonObject &message)
{
    QString r = message.value("info").toObject().value("role").toString();   // V1: {info:{role}, parts:[]}
    if (r.isEmpty())
        r = message.value("role").toString();
    if (r.isEmpty())
        r = message.value("type").toString();                                // V2 projected: type = user|assistant|...
    return r;
}

QJsonArray ConversationModel::partsOf(const QJsonObject &message)
{
    if (message.value("content").isArray())
        return message.value("content").toArray();
    if (message.value("parts").isArray())
        return message.value("parts").toArray();
    return {};
}

QString ConversationModel::textOfParts(const QJsonArray &parts)
{
    QStringList texts;
    for (const QJsonValue &v : parts) {
        const QJsonObject part = v.toObject();
        if (part.value("type").toString() == QLatin1String("text") && !part.value("synthetic").toBool()) {
            const QString t = part.value("text").toString();
            if (!t.isEmpty())
                texts << t;
        }
    }
    return texts.join(QStringLiteral("\n\n")).trimmed();
}

QString ConversationModel::statusOfTool(const QJsonObject &part)
{
    QString s = part.value("status").toString();
    if (s.isEmpty())
        s = part.value("state").toObject().value("status").toString();
    return normalizedToolStatus(s);
}

QString ConversationModel::summarizeInput(const QJsonValue &input, const QString &fallback)
{
    QString text;
    if (input.isString()) {
        text = input.toString();
    } else if (input.isObject()) {
        text = QString::fromUtf8(QJsonDocument(input.toObject()).toJson(QJsonDocument::Compact));
    } else if (input.isArray()) {
        text = QString::fromUtf8(QJsonDocument(input.toArray()).toJson(QJsonDocument::Compact));
    }
    if (text.isEmpty())
        return fallback;
    text.replace(QLatin1Char('\n'), QLatin1Char(' '));
    if (text.size() > 240)
        text = text.left(239) + QChar(0x2026);
    return text;
}

QString ConversationModel::toolDetailText(const QJsonValue &input, const QJsonValue &output, const QString &error)
{
    auto render = [](const QJsonValue &v) -> QString {
        if (v.isUndefined() || v.isNull())
            return {};
        if (v.isString())
            return v.toString();
        if (v.isObject())
            return QString::fromUtf8(QJsonDocument(v.toObject()).toJson(QJsonDocument::Indented));
        if (v.isArray())
            return QString::fromUtf8(QJsonDocument(v.toArray()).toJson(QJsonDocument::Indented));
        return v.toVariant().toString();
    };
    QStringList sections;
    const QString in = render(input);
    if (!in.isEmpty())
        sections << QStringLiteral("Input:\n") + in;
    const QString out = render(output);
    if (!out.isEmpty())
        sections << QStringLiteral("Output:\n") + out;
    if (!error.isEmpty())
        sections << QStringLiteral("Error:\n") + error;
    return sections.join(QStringLiteral("\n\n"));
}

QJsonValue ConversationModel::toolInputOf(const QJsonObject &part)
{
    if (part.contains(QStringLiteral("input")))
        return part.value(QStringLiteral("input"));
    return part.value(QStringLiteral("state")).toObject().value(QStringLiteral("input"));
}

QJsonValue ConversationModel::toolOutputOf(const QJsonObject &part)
{
    const QJsonObject state = part.value(QStringLiteral("state")).toObject();
    if (state.contains(QStringLiteral("output")))
        return state.value(QStringLiteral("output"));
    return part.value(QStringLiteral("output"));
}

void ConversationModel::addNotice(const QString &text)
{
    appendNotice(text);
}

void ConversationModel::applyUserPrompt(const QString &text)
{
    if (text.isEmpty())
        return;
    Item it;
    it.kind = QStringLiteral("user");
    it.id = QStringLiteral("user:local-%1").arg(++m_userPromptSeq);
    it.text = text;
    upsert(it);
}

void ConversationModel::clear()
{
    if (m_items.isEmpty())
        return;
    beginResetModel();
    m_items.clear();
    endResetModel();
    emit countChanged();
}

void ConversationModel::applyEvent(const QString &type, const QJsonObject &p)
{
    // ---- V2 streamed text / reasoning ---------------------------------------------------------------------------
    if (type == QLatin1String("session.text.started") || type == QLatin1String("session.text.delta")
        || type == QLatin1String("session.text.ended")) {
        const QString msg = p.value("assistantMessageID").toString();
        const QString id = QStringLiteral("text:%1#%2")
                               .arg(msg.isEmpty() ? QStringLiteral("?") : msg)
                               .arg(p.value("ordinal").toInt());
        Item it;
        it.kind = QStringLiteral("assistant-text");
        it.id = id;
        const int row = upsert(it);
        if (type == QLatin1String("session.text.delta")) {
            m_items[row].text += p.value("delta").toString();
            touch(row);
            setStreaming(row, true);
        } else if (type == QLatin1String("session.text.ended")) {
            if (p.contains(QStringLiteral("text")))
                m_items[row].text = p.value("text").toString();     // authoritative full block
            touch(row);
            setStreaming(row, false);
        } else {
            setStreaming(row, true);
        }
        return;
    }
    if (type == QLatin1String("session.reasoning.started") || type == QLatin1String("session.reasoning.delta")
        || type == QLatin1String("session.reasoning.ended")) {
        const QString msg = p.value("assistantMessageID").toString();
        const QString id = QStringLiteral("reason:%1#%2")
                               .arg(msg.isEmpty() ? QStringLiteral("?") : msg)
                               .arg(p.value("ordinal").toInt());
        Item it;
        it.kind = QStringLiteral("reasoning");
        it.id = id;
        const int row = upsert(it);
        if (type == QLatin1String("session.reasoning.delta")) {
            m_items[row].text += p.value("delta").toString();
            touch(row);
            setStreaming(row, true);
        } else if (type == QLatin1String("session.reasoning.ended")) {
            if (p.contains(QStringLiteral("text")))
                m_items[row].text = p.value("text").toString();
            touch(row);
            setStreaming(row, false);
        } else {
            setStreaming(row, true);
        }
        return;
    }

    // ---- tools (V2) ------------------------------------------------------------------------------------------------
    if (type.startsWith(QLatin1String("session.tool."))) {
        const QString key = firstString(p, {"id", "callID", "toolCallID", "toolID"});
        Item it;
        it.kind = QStringLiteral("tool");
        it.id = QStringLiteral("tool:%1").arg(key.isEmpty() ? QStringLiteral("?") : key);
        it.name = firstString(p, {"name", "tool", "toolName"});
        const QJsonValue input = p.value(QStringLiteral("input"));
        const QJsonValue output = p.contains(QStringLiteral("output")) ? p.value(QStringLiteral("output"))
                                                                        : p.value(QStringLiteral("result"));
        if (type == QLatin1String("session.tool.success")) {
            it.status = QStringLiteral("ok");
            it.text = toolDetailText(input, output);
        } else if (type == QLatin1String("session.tool.failed")) {
            it.status = QStringLiteral("error");
            const QJsonValue err = p.value("error");
            const QString message = err.isString() ? err.toString() : firstString(err.toObject(), {"message", "name"});
            it.detail = message;
            it.text = toolDetailText(input, output, message);
        } else {
            it.status = QStringLiteral("running");
            if (p.contains(QStringLiteral("input")))
                it.detail = summarizeInput(p.value("input"), it.detail);
            it.text = toolDetailText(input, output);
        }
        upsert(it);
        return;
    }
    if (type == QLatin1String("session.shell.started") || type == QLatin1String("session.shell.ended")) {
        Item it;
        it.kind = QStringLiteral("tool");
        it.id = QStringLiteral("tool:shell");
        it.name = QStringLiteral("shell");
        it.status = type == QLatin1String("session.shell.ended") ? QStringLiteral("ok") : QStringLiteral("running");
        upsert(it);
        return;
    }

    // ---- permission requests / forms --------------------------------------------------------------------------------
    if (type == QLatin1String("permission.asked") || type == QLatin1String("permission.updated")) {
        const QString id = firstString(p, {"id", "requestID", "permissionID"});
        QString title = firstString(p, {"title", "message", "permission"});
        if (title.isEmpty()) {
            const QString action = firstString(p, {"action"});
            const QJsonArray res = p.value("resources").toArray();
            title = res.isEmpty() ? action : action + QStringLiteral(": ") + res.first().toString();
        }
        Item it;
        it.kind = QStringLiteral("request");
        it.id = QStringLiteral("req:%1").arg(id);
        it.text = title;
        it.name = firstString(p, {"action", "title"});
        it.status = QStringLiteral("pending");
        it.requestKind = QStringLiteral("permission");
        upsert(it);
        return;
    }
    if (type == QLatin1String("permission.replied") || type == QLatin1String("form.replied")
        || type == QLatin1String("form.cancelled")) {
        const QString id = firstString(p, {"id", "requestID", "formID"});
        if (!id.isEmpty() && indexOf(QStringLiteral("request"), QStringLiteral("req:%1").arg(id)) >= 0) {
            Item it;
            it.kind = QStringLiteral("request");
            it.id = QStringLiteral("req:%1").arg(id);
            it.status = type == QLatin1String("form.cancelled") ? QStringLiteral("cancelled")
                                                                : QStringLiteral("answered");
            upsert(it);
        }
        return;
    }
    if (type == QLatin1String("form.created")) {
        Item it;
        it.kind = QStringLiteral("request");
        it.id = QStringLiteral("req:%1").arg(firstString(p, {"id", "formID"}));
        it.text = firstString(p, {"title", "message"}).isEmpty() ? QStringLiteral("opencode asks a question")
                                                                 : firstString(p, {"title", "message"});
        it.status = QStringLiteral("pending");
        it.requestKind = QStringLiteral("form");
        upsert(it);
        return;
    }

    // ---- notices ---------------------------------------------------------------------------------------------------
    if (type == QLatin1String("session.error") || type == QLatin1String("session.execution.failed")) {
        const QJsonObject err = p.value("error").toObject();
        appendNotice(firstString(err, {"message", "name", "type"}).isEmpty() ? firstString(p, {"message"})
                                                                            : firstString(err, {"message", "name", "type"}));
        return;
    }
    if (type == QLatin1String("session.compaction.started") || type == QLatin1String("session.compaction.ended")
        || type == QLatin1String("session.compaction.failed")) {
        Item it;
        it.kind = QStringLiteral("notice");
        it.id = QStringLiteral("notice:compaction");
        it.text = type == QLatin1String("session.compaction.failed") ? QStringLiteral("Compacting failed")
                 : type == QLatin1String("session.compaction.ended") ? QStringLiteral("Compacted")
                                                                    : QStringLiteral("Compacting…");
        it.status = type == QLatin1String("session.compaction.failed") ? QStringLiteral("error")
                  : type == QLatin1String("session.compaction.ended") ? QStringLiteral("ok")
                                                                     : QStringLiteral("running");
        upsert(it);
        return;
    }
    if (type == QLatin1String("session.retry.scheduled")) {
        Item it;
        it.kind = QStringLiteral("notice");
        it.id = QStringLiteral("notice:retry");
        it.text = QStringLiteral("Retrying…");
        it.status = QStringLiteral("running");
        upsert(it);
        return;
    }

    // ---- V1 part events --------------------------------------------------------------------------------------------
    if (type == QLatin1String("message.part.updated")) {
        const QJsonObject part = p.value("part").toObject();
        const QString pt = part.value("type").toString();
        const QString pid = firstString(part, {"id", "partID"});
        if (pt == QLatin1String("text") && !part.value("synthetic").toBool()) {
            Item it;
            it.kind = QStringLiteral("assistant-text");
            it.id = QStringLiteral("text:%1").arg(pid);
            it.text = part.value("text").toString();
            upsert(it);
        } else if (pt == QLatin1String("reasoning")) {
            Item it;
            it.kind = QStringLiteral("reasoning");
            it.id = QStringLiteral("reason:%1").arg(pid);
            it.text = part.value("text").toString();
            upsert(it);
        } else if (pt == QLatin1String("tool")) {
            const QString key = firstString(part, {"id", "callID", "toolCallID"});
            Item it;
            it.kind = QStringLiteral("tool");
            it.id = QStringLiteral("tool:%1").arg(key.isEmpty() ? pid : key);
            it.name = firstString(part, {"tool", "name"});
            it.status = statusOfTool(part);
            it.detail = summarizeInput(part.value("state").toObject().value("input"), it.detail);
            it.text = toolDetailText(toolInputOf(part), toolOutputOf(part));
            upsert(it);
        }
        return;
    }
    if (type == QLatin1String("message.part.delta")) {
        if (p.value("field").toString() == QLatin1String("text")) {
            const QString pid = firstString(p, {"partID", "id"});
            Item it;
            it.kind = QStringLiteral("assistant-text");
            it.id = QStringLiteral("text:%1").arg(pid);
            const int row = upsert(it);
            m_items[row].text += p.value("delta").toString();
            touch(row);
            setStreaming(row, true);
        }
        return;
    }
}

void ConversationModel::applyMessages(const QJsonArray &asc)
{
    QList<Item> pending;
    for (const Item &it : std::as_const(m_items))
        if (it.kind == QLatin1String("request") && it.status == QLatin1String("pending"))
            pending << it;

    QList<Item> out;
    int messageIndex = 0;
    for (const QJsonValue &value : asc) {
        const QJsonObject message = value.toObject();
        const QString role = roleOf(message);
        QString msgId = message.value("id").toString();
        if (msgId.isEmpty())
            msgId = message.value("info").toObject().value("id").toString();
        if (msgId.isEmpty())
            msgId = QStringLiteral("msg%1").arg(messageIndex);
        const QJsonArray parts = partsOf(message);

        if (role == QLatin1String("user")) {
            QString text = textOfParts(parts);
            if (text.isEmpty() && message.value("content").isString())   // some servers send plain-string content
                text = message.value("content").toString().trimmed();
            if (!text.isEmpty())
                out << Item{QStringLiteral("user"), QStringLiteral("user:%1").arg(msgId), text, {}, {}, {}, {}, false};
        } else if (role == QLatin1String("assistant")) {
            int ordinal = 0;
            for (const QJsonValue &pv : parts) {
                const QJsonObject part = pv.toObject();
                const QString pt = part.value("type").toString();
                const QString key = QStringLiteral("%1#%2").arg(msgId).arg(ordinal++);
                if (pt == QLatin1String("text") && !part.value("synthetic").toBool()) {
                    QString t = part.value("text").toString();
                    if (!t.isEmpty())
                        out << Item{QStringLiteral("assistant-text"), QStringLiteral("text:%1").arg(key), t,
                                    {}, {}, {}, {}, false};
                } else if (pt == QLatin1String("reasoning")) {
                    QString t = part.value("text").toString();
                    if (t.isEmpty())
                        t = part.value("content").toString();
                    if (!t.isEmpty())
                        out << Item{QStringLiteral("reasoning"), QStringLiteral("reason:%1").arg(key), t,
                                    {}, {}, {}, {}, false};
                } else if (pt == QLatin1String("tool")) {
                    const QString toolId = firstString(part, {"id", "callID", "toolCallID"});
                    const QString name = firstString(part, {"name", "tool"});
                    const QString detail = summarizeInput(part.value("input"),
                                                          summarizeInput(part.value("state").toObject().value("input"), {}));
                    out << Item{QStringLiteral("tool"),
                                QStringLiteral("tool:%1").arg(toolId.isEmpty() ? key : toolId),
                                toolDetailText(toolInputOf(part), toolOutputOf(part)), name, statusOfTool(part), {}, detail, false};
                }
            }
        }
        ++messageIndex;
    }

    // The projection never contains permission/form requests - keep the ones still waiting.
    for (const Item &p : std::as_const(pending))
        out << p;

    beginResetModel();
    m_items = out;
    endResetModel();
    emit countChanged();
}
