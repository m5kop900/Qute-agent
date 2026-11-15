#include "sseparser.h"

QList<SseEvent> SseParser::feed(const QByteArray &chunk)
{
    QList<SseEvent> out;
    m_buffer.append(chunk);

    for (;;) {
        const qsizetype nl = m_buffer.indexOf('\n');
        if (nl < 0)
            break;
        QByteArray line = m_buffer.left(nl);
        m_buffer.remove(0, nl + 1);
        if (line.endsWith('\r'))
            line.chop(1);

        if (line.isEmpty()) {                       // blank line => dispatch
            if (m_hasData || !m_event.isEmpty()) {
                out.append(SseEvent{m_event, m_data, m_id});
            }
            m_event.clear();
            m_data.clear();
            m_hasData = false;
            continue;
        }
        if (line.startsWith(':'))                   // comment / keep-alive
            continue;

        QByteArray field = line;
        QByteArray value;
        const qsizetype colon = line.indexOf(':');
        if (colon >= 0) {
            field = line.left(colon);
            value = line.mid(colon + 1);
            if (value.startsWith(' '))
                value.remove(0, 1);
        }

        if (field == "event") {
            m_event = QString::fromUtf8(value);
        } else if (field == "data") {
            if (m_hasData)
                m_data.append('\n');
            m_data.append(value);
            m_hasData = true;
        } else if (field == "id") {
            m_id = QString::fromUtf8(value);
        }
        // "retry" and unknown fields are ignored.
    }
    return out;
}

void SseParser::reset()
{
    m_buffer.clear();
    m_event.clear();
    m_data.clear();
    m_id.clear();
    m_hasData = false;
}
