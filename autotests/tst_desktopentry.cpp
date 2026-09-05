// SPDX-License-Identifier: MIT
#include <QCoreApplication>
#include <QFile>
#include <QTest>

#include "DesktopEntry.h"
#include "WebApp.h"

#include "common/TempHome.h"

class TstDesktopEntry : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase()
    {
        QVERIFY(m_home.isValid());
    }

    void write_and_parse_roundTrip()
    {
        const QString path =
            m_home.applicationsDir() + QStringLiteral("/WebApp-Test1234.desktop");

        WebApp app;
        app.setPath(path);
        app.setCodename(QStringLiteral("Test1234"));
        app.setName(QStringLiteral("Notion"));
        app.setDescription(QStringLiteral("Notas"));
        app.setUrl(QStringLiteral("https://app.notion.com"));
        app.setIcon(QStringLiteral("notion"));
        app.setCategory(QStringLiteral("Network"));
        app.setBrowserName(TempHome::fakeBrowserName());
        app.setCustomParameters(QStringLiteral("--start-maximized"));
        app.setIsolateProfile(true);
        app.setNavbar(false);
        app.setPrivateWindow(false);

        const QString exec =
            QStringLiteral(
                "chromium --app=\"https://app.notion.com\" "
                "--class=WebApp-Test1234 --name=WebApp-Test1234 "
                "--user-data-dir=/tmp/profile --start-maximized");

        QVERIFY(DesktopEntry::write(app, exec));

        QFile file(path);
        QVERIFY(file.open(QIODevice::ReadOnly | QIODevice::Text));
        const QString content = QString::fromUtf8(file.readAll());
        file.close();

        QVERIFY(content.contains(QStringLiteral("[Desktop Entry]")));
        QVERIFY(!content.contains(QStringLiteral("Desktop%20Entry")));
        QVERIFY(content.contains(QStringLiteral("Categories=Network;")));
        QVERIFY(!content.contains(QStringLiteral("GTK;")));
        QVERIFY(content.contains(QStringLiteral("Exec=chromium --app=")));
        QVERIFY(content.contains(QStringLiteral("--start-maximized")));

        const WebApp parsed = DesktopEntry::parse(path, QStringLiteral("Test1234"));
        QVERIFY(parsed.isValid());
        QCOMPARE(parsed.name(), QStringLiteral("Notion"));
        QCOMPARE(parsed.category(), QStringLiteral("Network"));
        QCOMPARE(parsed.customParameters(), QStringLiteral("--start-maximized"));
        QCOMPARE(parsed.exec(), exec);
        QVERIFY(parsed.isolateProfile());
    }

    void updateFields_does_not_corrupt_exec()
    {
        const QString path =
            m_home.applicationsDir()
            + QStringLiteral("/WebApp-Edit5678.desktop");

        WebApp app;
        app.setPath(path);
        app.setCodename(QStringLiteral("Edit5678"));
        app.setName(QStringLiteral("Mail"));
        app.setUrl(QStringLiteral("https://mail.example.com"));
        app.setIcon(QStringLiteral("mail"));
        app.setCategory(QStringLiteral("Network"));
        app.setBrowserName(TempHome::fakeBrowserName());
        app.setCustomParameters(QStringLiteral("--start-maximized"));
        app.setIsolateProfile(true);

        const QString execIsolated =
            QStringLiteral(
                "chromium --app=\"https://mail.example.com\" "
                "--class=WebApp-Edit5678 --name=WebApp-Edit5678 "
                "--user-data-dir=/tmp/p --start-maximized");
        QVERIFY(DesktopEntry::write(app, execIsolated));

        app.setIsolateProfile(false);
        app.setCategory(QStringLiteral("Utility"));
        const QString execShared =
            QStringLiteral(
                "chromium --app=\"https://mail.example.com\" "
                "--class=WebApp-Edit5678 --name=WebApp-Edit5678 "
                "--start-maximized");
        QVERIFY(DesktopEntry::updateFields(app, execShared));

        QFile file(path);
        QVERIFY(file.open(QIODevice::ReadOnly | QIODevice::Text));
        const QString content = QString::fromUtf8(file.readAll());
        file.close();

        QVERIFY(content.startsWith(QStringLiteral("[Desktop Entry]")));
        QVERIFY(!content.contains(QStringLiteral("Desktop%20Entry")));
        QVERIFY(!content.contains(QStringLiteral("Exec=\"")));
        QVERIFY(content.contains(QStringLiteral("Categories=Utility;")));
        QVERIFY(content.contains(QStringLiteral("X-WebApp-Isolated=false")));
        QVERIFY(content.contains(QStringLiteral("--start-maximized")));
        QVERIFY(!content.contains(QStringLiteral(" start-maximized")));

        const WebApp parsed =
            DesktopEntry::parse(path, QStringLiteral("Edit5678"));
        QVERIFY(parsed.isValid());
        QCOMPARE(parsed.exec(), execShared);
        QVERIFY(!parsed.isolateProfile());
        QCOMPARE(parsed.category(), QStringLiteral("Utility"));
    }

    void parse_maps_legacy_webapps_category()
    {
        const QString path =
            m_home.applicationsDir()
            + QStringLiteral("/WebApp-Legacy9999.desktop");
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
        file.write(
            "[Desktop Entry]\n"
            "Name=Legacy\n"
            "Icon=legacy\n"
            "Exec=chromium --app=https://example.com\n"
            "Categories=GTK;WebApps;\n"
            "StartupWMClass=WebApp-Legacy9999\n"
            "X-WebApp-URL=https://example.com\n"
            "X-WebApp-Browser=Chromium\n"
            "X-WebApp-Isolated=true\n");
        file.close();

        const WebApp parsed =
            DesktopEntry::parse(path, QStringLiteral("Legacy9999"));
        QVERIFY(parsed.isValid());
        QCOMPARE(parsed.category(), QStringLiteral("Network"));
    }

private:
    TempHome m_home;
};

QTEST_GUILESS_MAIN(TstDesktopEntry)
#include "tst_desktopentry.moc"
