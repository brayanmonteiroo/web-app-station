// SPDX-License-Identifier: MIT
#include <QDir>
#include <QFile>
#include <QTest>

#include "Browser.h"
#include "ExecBuilder.h"
#include "Paths.h"
#include "ProfileService.h"

#include "common/TempHome.h"

class TstFirefoxProfile : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase() { QVERIFY(m_home.isValid()); }

    void titlebar_pref_and_chrome_css()
    {
        const QString profile =
            QDir(Paths::firefoxProfilesDir()).filePath(QStringLiteral("UiTest"));
        ProfileService::ensureFirefoxProfile(profile, false, false);

        QFile userJs(QDir(profile).filePath(QStringLiteral("user.js")));
        QVERIFY(userJs.open(QIODevice::ReadOnly | QIODevice::Text));
        const QString prefs = QString::fromUtf8(userJs.readAll());
        QVERIFY(prefs.contains(
            QStringLiteral("user_pref(\"browser.tabs.inTitlebar\", 0)")));

        QFile css(QDir(profile).filePath(QStringLiteral("chrome/userChrome.css")));
        QVERIFY(css.open(QIODevice::ReadOnly | QIODevice::Text));
        const QString chrome = QString::fromUtf8(css.readAll());
        QVERIFY(chrome.contains(QStringLiteral("#TabsToolbar")));
        QVERIFY(chrome.contains(QStringLiteral("collapse")));
        QVERIFY(!QFile::exists(
            QDir(profile).filePath(QStringLiteral("xulstore.json"))));
    }

    void start_maximized_writes_xulstore()
    {
        const QString profile =
            QDir(Paths::firefoxProfilesDir())
                .filePath(QStringLiteral("MaxTest"));
        ProfileService::ensureFirefoxProfile(profile, false, true);

        QFile xul(QDir(profile).filePath(QStringLiteral("xulstore.json")));
        QVERIFY(xul.open(QIODevice::ReadOnly | QIODevice::Text));
        const QString body = QString::fromUtf8(xul.readAll());
        QVERIFY(body.contains(QStringLiteral("sizemode")));
        QVERIFY(body.contains(QStringLiteral("maximized")));
    }

    void exec_strips_chromium_maximize_flag()
    {
        Browser browser;
        browser.family = BrowserFamily::Firefox;
        browser.name = QStringLiteral("Firefox");
        browser.execPath = QStringLiteral("firefox");

        const QString exec = ExecBuilder::build(
            browser, QStringLiteral("FxMax"),
            QStringLiteral("https://app.notion.com"),
            QStringLiteral("notion"), QStringLiteral("--start-maximized"), false,
            false, false);

        QVERIFY(!exec.contains(QStringLiteral("--start-maximized")));
        QVERIFY(exec.contains(QStringLiteral("firefox")));
        QVERIFY(exec.contains(QStringLiteral("--profile")));

        const QString profile =
            QDir(Paths::firefoxProfilesDir()).filePath(QStringLiteral("FxMax"));
        QVERIFY(QFile::exists(
            QDir(profile).filePath(QStringLiteral("xulstore.json"))));
        QFile userJs(QDir(profile).filePath(QStringLiteral("user.js")));
        QVERIFY(userJs.open(QIODevice::ReadOnly | QIODevice::Text));
        QVERIFY(QString::fromUtf8(userJs.readAll())
                    .contains(QStringLiteral("browser.tabs.inTitlebar\", 0)")));
    }

    void exec_maps_fullscreen_to_kiosk()
    {
        Browser browser;
        browser.family = BrowserFamily::Firefox;
        browser.name = QStringLiteral("Firefox");
        browser.execPath = QStringLiteral("firefox");

        const QString exec = ExecBuilder::build(
            browser, QStringLiteral("FxFs"),
            QStringLiteral("https://example.com"), QStringLiteral("icon"),
            QStringLiteral("--start-fullscreen"), false, false, false);

        QVERIFY(exec.contains(QStringLiteral("--kiosk")));
        QVERIFY(!exec.contains(QStringLiteral("--start-fullscreen")));
    }

private:
    TempHome m_home;
};

QTEST_GUILESS_MAIN(TstFirefoxProfile)
#include "tst_firefoxprofile.moc"
