// SPDX-License-Identifier: MIT
#include <QTest>

#include "Browser.h"
#include "ExecBuilder.h"
#include "Paths.h"
#include "ProfileService.h"

#include "common/TempHome.h"

#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>

class TstExecBuilder : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase() { QVERIFY(m_home.isValid()); }

    void chromium_wmclass_matches_hub_format()
    {
        QCOMPARE(ExecBuilder::chromiumWmClass(QStringLiteral("Chrome"),
                                              QStringLiteral("https://app.notion.com")),
                 QStringLiteral("chrome-app.notion.com__-Default"));
        QCOMPARE(ExecBuilder::chromiumWmClass(QStringLiteral("Brave Browser"),
                                              QStringLiteral("https://app.notion.com/")),
                 QStringLiteral("brave-app.notion.com__-Default"));
        QCOMPARE(ExecBuilder::chromiumWmClass(QStringLiteral("Chrome"),
                                              QStringLiteral("https://some.nice.url")),
                 QStringLiteral("chrome-some.nice.url__-Default"));
    }

    void chromium_isolated_includes_user_data_dir()
    {
        Browser browser;
        browser.family = BrowserFamily::Chromium;
        browser.name = TempHome::fakeBrowserName();
        browser.execPath = m_home.fakeChromiumPath();

        const QString exec = ExecBuilder::build(
            browser, QStringLiteral("Code1234"),
            QStringLiteral("https://app.notion.com"),
            QStringLiteral("notion"), QStringLiteral("--start-maximized"), true,
            false, false);

        QVERIFY(exec.contains(QStringLiteral("--app=\"https://app.notion.com\"")));
        QVERIFY(exec.contains(QStringLiteral("--user-data-dir=")));
        QVERIFY(exec.contains(Paths::chromiumProfilesDir()));
        QVERIFY(exec.contains(QStringLiteral("--start-maximized")));
        QVERIFY(exec.contains(
            QStringLiteral("--class=chrome-app.notion.com__-Default")));
        QVERIFY(exec.contains(QStringLiteral("--no-first-run")));
        QVERIFY(!exec.contains(QStringLiteral("CHROME_DESKTOP=")));
        // Como no Mint: parâmetros extras depois de --app=
        QVERIFY(exec.indexOf(QStringLiteral("--app="))
                < exec.indexOf(QStringLiteral("--start-maximized")));
    }

    void chromium_normalizes_single_dash_start_maximized()
    {
        Browser browser;
        browser.family = BrowserFamily::Chromium;
        browser.name = TempHome::fakeBrowserName();
        browser.execPath = m_home.fakeChromiumPath();

        const QString exec = ExecBuilder::build(
            browser, QStringLiteral("Code1234"),
            QStringLiteral("https://app.notion.com"),
            QStringLiteral("notion"), QStringLiteral("-start-maximized"), false,
            false, false);

        QVERIFY(exec.contains(QStringLiteral("--start-maximized")));
        QVERIFY(!exec.contains(QStringLiteral(" -start-maximized")));
        QVERIFY(exec.indexOf(QStringLiteral("--app="))
                < exec.indexOf(QStringLiteral("--start-maximized")));
    }

    void chromium_shared_profile_omits_user_data_dir()
    {
        Browser browser;
        browser.family = BrowserFamily::Chromium;
        browser.name = TempHome::fakeBrowserName();
        browser.execPath = m_home.fakeChromiumPath();

        const QString exec = ExecBuilder::build(
            browser, QStringLiteral("Code1234"),
            QStringLiteral("https://app.notion.com"),
            QStringLiteral("notion"), QStringLiteral("--start-maximized"), false,
            false, false);

        QVERIFY(exec.contains(QStringLiteral("--app=\"https://app.notion.com\"")));
        QVERIFY(!exec.contains(QStringLiteral("--user-data-dir=")));
        QVERIFY(exec.contains(
            QStringLiteral("--class=chrome-app.notion.com__-Default")));
        QVERIFY(exec.contains(QStringLiteral("--start-maximized")));
        QVERIFY(exec.indexOf(QStringLiteral("--app="))
                < exec.indexOf(QStringLiteral("--start-maximized")));
    }

    void chromium_isolated_writes_maximized_preferences()
    {
        Browser browser;
        browser.family = BrowserFamily::Chromium;
        browser.name = TempHome::fakeBrowserName();
        browser.execPath = m_home.fakeChromiumPath();

        const QString exec = ExecBuilder::build(
            browser, QStringLiteral("ChrMax"),
            QStringLiteral("https://app.notion.com"),
            QStringLiteral("notion"), QStringLiteral("--start-maximized"), true,
            false, false);

        QVERIFY(exec.contains(QStringLiteral("--start-maximized")));
        QVERIFY(exec.contains(QStringLiteral("--user-data-dir=")));

        const QString prefsPath =
            QDir(Paths::chromiumProfilesDir())
                .filePath(QStringLiteral("ChrMax/Default/Preferences"));
        QVERIFY(QFile::exists(prefsPath));
        QVERIFY(preferencesHasMaximized(prefsPath));
    }

    void chromium_shared_maximized_writes_browser_preferences()
    {
        Browser browser;
        browser.family = BrowserFamily::Chromium;
        browser.name = TempHome::fakeBrowserName();
        browser.execPath = m_home.fakeChromiumPath();

        const QString exec = ExecBuilder::build(
            browser, QStringLiteral("SharedMax"),
            QStringLiteral("https://app.notion.com"),
            QStringLiteral("notion"), QStringLiteral("--start-maximized"), false,
            false, false);
        QVERIFY(exec.contains(QStringLiteral("--start-maximized")));
        QVERIFY(!exec.contains(QStringLiteral("--user-data-dir=")));

        // Sem isolado: Preferences do user-data do browser, não do ICE.
        QVERIFY(!QFile::exists(
            QDir(Paths::chromiumProfilesDir())
                .filePath(QStringLiteral("SharedMax/Default/Preferences"))));

        const QString sharedDir = ProfileService::chromiumSharedUserDataDir(
            browser.name, browser.execPath);
        QVERIFY(sharedDir.contains(QStringLiteral("org.chromium.Chromium")));
        const QString prefsPath =
            QDir(sharedDir).filePath(QStringLiteral("Default/Preferences"));
        QVERIFY(QFile::exists(prefsPath));
        QVERIFY(preferencesHasMaximized(prefsPath));
    }

    void chromium_shared_without_maximize_skips_preferences()
    {
        Browser browser;
        browser.family = BrowserFamily::Chromium;
        browser.name = QStringLiteral("Chrome");
        browser.execPath = QStringLiteral("google-chrome-stable");

        const QString sharedDir = ProfileService::chromiumSharedUserDataDir(
            browser.name, browser.execPath);
        const QString prefsPath =
            QDir(sharedDir).filePath(QStringLiteral("Default/Preferences"));
        QFile::remove(prefsPath);

        const QString exec = ExecBuilder::build(
            browser, QStringLiteral("NoMax"),
            QStringLiteral("https://app.notion.com"),
            QStringLiteral("notion"), {}, false, false, false);

        QVERIFY(!exec.contains(QStringLiteral("--start-maximized")));
        QVERIFY(!exec.contains(QStringLiteral("--user-data-dir=")));
        QVERIFY(!QFile::exists(prefsPath));
    }

    void chromium_shared_user_data_dir_resolves_chrome_and_brave()
    {
        const QString config =
            QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
        QCOMPARE(ProfileService::chromiumSharedUserDataDir(
                     QStringLiteral("Chrome"),
                     QStringLiteral("google-chrome-stable")),
                 config + QStringLiteral("/google-chrome"));
        QCOMPARE(ProfileService::chromiumSharedUserDataDir(
                     QStringLiteral("Brave Browser"),
                     QStringLiteral("/usr/bin/brave-browser")),
                 config
                     + QStringLiteral(
                         "/BraveSoftware/Brave-Browser"));
    }

    void chromium_maximized_merges_existing_preferences()
    {
        const QString userData =
            QDir(m_home.homePath()).filePath(QStringLiteral("chrome-merge"));
        const QString defaultDir =
            QDir(userData).filePath(QStringLiteral("Default"));
        QDir().mkpath(defaultDir);
        const QString prefsPath =
            QDir(defaultDir).filePath(QStringLiteral("Preferences"));
        {
            QFile f(prefsPath);
            QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Truncate));
            f.write(R"({"profile":{"name":"Person 1"},"browser":{"window_placement":{"maximized":false,"left":10}}})");
        }

        ProfileService::ensureChromiumMaximized(userData);
        QVERIFY(preferencesHasMaximized(prefsPath));

        QFile f(prefsPath);
        QVERIFY(f.open(QIODevice::ReadOnly));
        const QJsonObject root =
            QJsonDocument::fromJson(f.readAll()).object();
        QCOMPARE(root.value(QStringLiteral("profile"))
                     .toObject()
                     .value(QStringLiteral("name"))
                     .toString(),
                 QStringLiteral("Person 1"));
        QCOMPARE(root.value(QStringLiteral("browser"))
                     .toObject()
                     .value(QStringLiteral("window_placement"))
                     .toObject()
                     .value(QStringLiteral("left"))
                     .toInt(),
                 10);
    }

    void startup_wmclass_detects_chromium_by_name()
    {
        QCOMPARE(ExecBuilder::startupWmClass(QStringLiteral("Chrome"),
                                             QStringLiteral("Code1"),
                                             QStringLiteral("https://app.notion.com")),
                 QStringLiteral("chrome-app.notion.com__-Default"));
        QCOMPARE(ExecBuilder::startupWmClass(QStringLiteral("Firefox"),
                                             QStringLiteral("Fx1"),
                                             QStringLiteral("https://a.b")),
                 QStringLiteral("WebApp-Fx1"));
    }

    void chromium_isolated_keeps_user_data_dir()
    {
        Browser browser;
        browser.family = BrowserFamily::Chromium;
        browser.name = TempHome::fakeBrowserName();
        browser.execPath = m_home.fakeChromiumPath();

        const QString exec = ExecBuilder::build(
            browser, QStringLiteral("Iso1"),
            QStringLiteral("https://example.com"), QStringLiteral("icon"), {},
            true, false, false);

        QVERIFY(exec.contains(QStringLiteral("--user-data-dir=")));
        QVERIFY(exec.contains(
            QStringLiteral("--class=chrome-example.com__-Default")));
    }

    void chromium_private_window_flag()
    {
        Browser browser;
        browser.family = BrowserFamily::Chromium;
        browser.name = QStringLiteral("Chrome");
        browser.execPath = QStringLiteral("google-chrome-stable");

        const QString exec = ExecBuilder::build(
            browser, QStringLiteral("X1"), QStringLiteral("https://x.test"),
            QStringLiteral("icon"), {}, false, false, true);

        QVERIFY(exec.contains(QStringLiteral("--incognito")));
    }

private:
    static bool preferencesHasMaximized(const QString &prefsPath)
    {
        QFile prefs(prefsPath);
        if (!prefs.open(QIODevice::ReadOnly)) {
            return false;
        }
        const QJsonObject browser =
            QJsonDocument::fromJson(prefs.readAll())
                .object()
                .value(QStringLiteral("browser"))
                .toObject();
        return browser.value(QStringLiteral("window_placement"))
                   .toObject()
                   .value(QStringLiteral("maximized"))
                   .toBool(false)
            && browser.value(QStringLiteral("window_placement_popup"))
                   .toObject()
                   .value(QStringLiteral("maximized"))
                   .toBool(false);
    }

    TempHome m_home;
};

QTEST_GUILESS_MAIN(TstExecBuilder)
#include "tst_execbuilder.moc"
