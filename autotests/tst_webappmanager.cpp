// SPDX-License-Identifier: MIT
#include <QDir>
#include <QFile>
#include <QRegularExpression>
#include <QTest>

#include "Browser.h"
#include "BrowserDetector.h"
#include "DesktopEntry.h"
#include "WebAppManager.h"

#include "common/TempHome.h"

class TstWebAppManager : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase()
    {
        QVERIFY(m_home.isValid());
        BrowserDetector detector;
        QVERIFY2(!detector.installedBrowsers().isEmpty(),
                 "Fake Chromium Flatpak should be detected");
    }

    void create_lists_and_delete()
    {
        WebAppManager manager;
        QCOMPARE(manager.webApps().size(), 0);

        Browser browser;
        browser.family = BrowserFamily::Chromium;
        browser.name = TempHome::fakeBrowserName();
        browser.execPath = m_home.fakeChromiumPath();

        QVERIFY(manager.createWebApp(
            QStringLiteral("Notion"), QStringLiteral("Notas"),
            QStringLiteral("app.notion.com"), QStringLiteral("notion"),
            QStringLiteral("Network"), browser,
            QStringLiteral("--start-maximized"), true, false, false));

        const QList<WebApp> apps = manager.webApps();
        QCOMPARE(apps.size(), 1);
        QCOMPARE(apps.at(0).name(), QStringLiteral("Notion"));
        QCOMPARE(apps.at(0).url(), QStringLiteral("http://app.notion.com"));
        QCOMPARE(apps.at(0).category(), QStringLiteral("Network"));
        QVERIFY(apps.at(0).isolateProfile());
        QVERIFY(apps.at(0).exec().contains(QStringLiteral("--user-data-dir=")));
        QVERIFY(apps.at(0).exec().contains(QStringLiteral("--start-maximized")));

        QFile desktop(apps.at(0).path());
        QVERIFY(desktop.open(QIODevice::ReadOnly | QIODevice::Text));
        const QString content = QString::fromUtf8(desktop.readAll());
        QVERIFY(content.contains(QStringLiteral("Categories=Network;")));
        QVERIFY(!content.contains(QStringLiteral("GTK;")));

        QVERIFY(manager.deleteWebApp(apps.at(0)));
        QCOMPARE(manager.webApps().size(), 0);
    }

    void edit_toggles_isolation_without_corrupting_desktop()
    {
        WebAppManager manager;
        Browser browser;
        browser.family = BrowserFamily::Chromium;
        browser.name = TempHome::fakeBrowserName();
        browser.execPath = m_home.fakeChromiumPath();

        QVERIFY(manager.createWebApp(
            QStringLiteral("EditMe"), {}, QStringLiteral("https://edit.test"),
            QStringLiteral("edit"), QStringLiteral("Network"), browser,
            QStringLiteral("--start-maximized"), true, false, false));

        WebApp existing = manager.webApps().constFirst();
        QVERIFY(manager.editWebApp(
            existing, QStringLiteral("EditMe"), {},
            QStringLiteral("https://edit.test"), QStringLiteral("edit"),
            QStringLiteral("Utility"), browser,
            QStringLiteral("--start-maximized"), false, false, false));

        const WebApp updated = manager.webApps().constFirst();
        QVERIFY(!updated.isolateProfile());
        QVERIFY(!updated.exec().contains(QStringLiteral("--user-data-dir=")));
        QVERIFY(updated.exec().contains(
            QStringLiteral("--class=chrome-edit.test__-Default")));
        QVERIFY(updated.exec().contains(QStringLiteral("--start-maximized")));
        QVERIFY(!updated.exec().contains(QStringLiteral("CHROME_DESKTOP=")));
        QCOMPARE(updated.category(), QStringLiteral("Utility"));

        QFile file(updated.path());
        QVERIFY(file.open(QIODevice::ReadOnly | QIODevice::Text));
        const QString content = QString::fromUtf8(file.readAll());
        QVERIFY(content.startsWith(QStringLiteral("[Desktop Entry]")));
        QVERIFY(!content.contains(QStringLiteral("Exec=\"")));
        QVERIFY(!content.contains(QStringLiteral("Desktop%20Entry")));
    }

    void launch_rebuilds_exec_from_metadata()
    {
        WebAppManager manager;
        Browser browser;
        browser.family = BrowserFamily::Chromium;
        browser.name = TempHome::fakeBrowserName();
        browser.execPath = m_home.fakeChromiumPath();

        QVERIFY(manager.createWebApp(
            QStringLiteral("LaunchMe"), {}, QStringLiteral("https://launch.test"),
            QStringLiteral("launch"), QStringLiteral("Network"), browser, {},
            false, false, false));

        WebApp app = manager.webApps().constFirst();
        // Corrompe o Exec como o QSettings antigo fazia.
        QFile file(app.path());
        QVERIFY(file.open(QIODevice::ReadOnly | QIODevice::Text));
        QString content = QString::fromUtf8(file.readAll());
        file.close();
        content.replace(QRegularExpression(QStringLiteral("^Exec=.*$"),
                                           QRegularExpression::MultilineOption),
                        QStringLiteral(
                            "Exec=\"broken --app=\\\"https://launch.test\\\"\""));
        QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Truncate
                          | QIODevice::Text));
        file.write(content.toUtf8());
        file.close();

        app = manager.webApps().constFirst();
        QVERIFY(manager.launchWebApp(app));
    }

    void normalizeUrl_adds_scheme()
    {
        QCOMPARE(WebAppManager::normalizeUrl(QStringLiteral("exemplo.com")),
                 QStringLiteral("http://exemplo.com"));
        QCOMPARE(WebAppManager::normalizeUrl(QStringLiteral("https://x.test")),
                 QStringLiteral("https://x.test"));
    }

private:
    TempHome m_home;
};

QTEST_GUILESS_MAIN(TstWebAppManager)
#include "tst_webappmanager.moc"
