// SPDX-License-Identifier: MIT
#include <QTest>

#include "Browser.h"
#include "ExecBuilder.h"
#include "Paths.h"

#include "common/TempHome.h"

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
        QVERIFY(exec.indexOf(QStringLiteral("--start-maximized"))
                < exec.indexOf(QStringLiteral("--app=")));
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
        QVERIFY(exec.indexOf(QStringLiteral("--start-maximized"))
                < exec.indexOf(QStringLiteral("--app=")));
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
        QVERIFY(exec.indexOf(QStringLiteral("--start-maximized"))
                < exec.indexOf(QStringLiteral("--app=")));
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
    TempHome m_home;
};

QTEST_GUILESS_MAIN(TstExecBuilder)
#include "tst_execbuilder.moc"
