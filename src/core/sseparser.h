#pragma once
#include <QByteArray>
#include <QList>
#include <QString>

// One dispatched Server-Sent Event.
struct SseEvent {
    QString event;      // "event:" field (may be empty)
    QByteArray data;    // "data:" lines joined with '\n'
    QString id;         // "id:" field (may be empty)
};

// Incremental SSE parser (https://html.spec.whatwg.org/multipage/server-sent-events.html).
// Feed it arbitrary chunks as they arrive; it returns the events that completed.
class SseParser {
public:
    QList<SseEvent> feed(const QByteArray &chunk);
    void reset();

private:
    QByteArray m_buffer;
    QString m_event;
    QByteArray m_data;
    QString m_id;
    bool m_hasData = false;
};
