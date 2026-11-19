// F7.1: the theme resolution is pure, so it is tested without a GUI session.
#include "theme.h"

#include <QtTest>

class TstTheme : public QObject {
    Q_OBJECT
private slots:
    void explicitModesWin()
    {
        QCOMPARE(Theme::resolveDark(QStringLiteral("dark"), Qt::ColorScheme::Light, false), true);
        QCOMPARE(Theme::resolveDark(QStringLiteral("dark"), Qt::ColorScheme::Unknown, false), true);
        QCOMPARE(Theme::resolveDark(QStringLiteral("light"), Qt::ColorScheme::Dark, true), false);
        QCOMPARE(Theme::resolveDark(QStringLiteral("light"), Qt::ColorScheme::Unknown, true), false);
    }
    void systemFollowsTheColorScheme()
    {
        QCOMPARE(Theme::resolveDark(QStringLiteral("system"), Qt::ColorScheme::Dark, false), true);
        QCOMPARE(Theme::resolveDark(QStringLiteral("system"), Qt::ColorScheme::Light, true), false);
    }
    void systemFallsBackToThePalette()
    {
        QCOMPARE(Theme::resolveDark(QStringLiteral("system"), Qt::ColorScheme::Unknown, true), true);
        QCOMPARE(Theme::resolveDark(QStringLiteral("system"), Qt::ColorScheme::Unknown, false), false);
    }
    void unknownModeIsTreatedAsSystem()
    {
        QCOMPARE(Theme::resolveDark(QStringLiteral("solarized"), Qt::ColorScheme::Dark, false), true);
        QCOMPARE(Theme::resolveDark(QString(), Qt::ColorScheme::Unknown, true), true);
    }
    void accentPrecedence()
    {
        QCOMPARE(Theme::resolveAccent(QStringLiteral("#112233"), QColor("#00ff00")), QColor("#112233"));
        QCOMPARE(Theme::resolveAccent(QString(), QColor("#00ff00")), QColor("#00ff00"));
        // invalid config value and no system accent -> the built-in fallback
        QCOMPARE(Theme::resolveAccent(QStringLiteral("not-a-color"), QColor()), QColor("#6ea8ff"));
    }
    void contrastTextStaysReadable()
    {
        QCOMPARE(Theme::contrastText(QColor("#000000")), QColor("#ffffff"));
        QCOMPARE(Theme::contrastText(QColor("#ffffff")), QColor("#10121c"));
        QCOMPARE(Theme::contrastText(QColor("#6ea8ff")), QColor("#10121c"));   // light accent -> dark text
        QCOMPARE(Theme::contrastText(QColor("#1a1c2b")), QColor("#ffffff"));   // dark accent -> light text
    }
};

QTEST_APPLESS_MAIN(TstTheme)
#include "tst_theme.moc"
