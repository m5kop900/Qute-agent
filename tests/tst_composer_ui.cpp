#include "appconfig.h"
#include "attachmentmodel.h"
#include "composertokens.h"
#include "opencodecatalog.h"
#include "opencodeclient.h"

#include <QQmlContext>
#include <QQmlEngine>
#include <QtQuickTest>

// Stubs for the parts of the overlay the prompt box talks to. The prompt box's own logic is real.
class FakeConversation : public QObject {
    Q_OBJECT
public:
    Q_INVOKABLE void addNotice(const QString &) {}
};

class FakeController : public QObject {
    Q_OBJECT
public:
    Q_INVOKABLE void hide() {}
};

class Setup : public QObject {
    Q_OBJECT
public slots:
    void qmlEngineAvailable(QQmlEngine *engine)
    {
        static AppConfig config;
        static OpenCodeClient client(&config);
        static OpenCodeCatalog catalog(&client);     // no server: built-in "/" items are still there
        static AttachmentModel attachments(&config);
        static ComposerTools tools;
        static FakeConversation conversation;
        static FakeController controller;
        QQmlContext *ctx = engine->rootContext();
        ctx->setContextProperty(QStringLiteral("catalog"), &catalog);
        ctx->setContextProperty(QStringLiteral("attachments"), &attachments);
        ctx->setContextProperty(QStringLiteral("composerTools"), &tools);
        ctx->setContextProperty(QStringLiteral("conversation"), &conversation);
        ctx->setContextProperty(QStringLiteral("controller"), &controller);
    }
};

QUICK_TEST_MAIN_WITH_SETUP(tst_composer_ui, Setup)

#include "tst_composer_ui.moc"
