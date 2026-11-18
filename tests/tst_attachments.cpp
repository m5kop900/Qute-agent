#include "appconfig.h"

#include <QtTest>

// F1.1: maxAttachmentSizeMB setting (0 = no limit). The per-file metadata helpers live in
// OverlayController (thin Qt wrappers around QFileInfo/QMimeData/QImage) and are verified
// manually against a live run; this test covers the setting itself.
class TstAttachments : public QObject {
    Q_OBJECT
private slots:
    void defaultIsUnlimited()
    {
        AppConfig config;
        QCOMPARE(config.maxAttachmentSizeMB(), 0);
    }

    void setterRoundTrip()
    {
        AppConfig config;
        config.setMaxAttachmentSizeMB(25);
        QCOMPARE(config.maxAttachmentSizeMB(), 25);
        config.setMaxAttachmentSizeMB(-5);   // clamped to 0 (unlimited)
        QCOMPARE(config.maxAttachmentSizeMB(), 0);
    }
};

QTEST_GUILESS_MAIN(TstAttachments)
#include "tst_attachments.moc"
