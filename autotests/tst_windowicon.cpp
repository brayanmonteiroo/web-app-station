// SPDX-License-Identifier: MIT
#include <QFile>
#include <QImage>
#include <QTest>

#include "DesktopEntry.h"
#include "Paths.h"
#include "ProfileService.h"
#include "WebApp.h"
#include "WebAppManager.h"
#include "Browser.h"

#include "common/TempHome.h"

class TstWindowIcon : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase() { QVERIFY(m_home.isValid()); }

    void install_hicolor_theme_icon()
    {
        QImage img(32, 32, QImage::Format_ARGB32);
        img.fill(Qt::blue);
        const QString src =
            m_home.homePath() + QStringLiteral("/src-icon.png");
        QVERIFY(img.save(src, "PNG"));

        const QString theme =
            ProfileService::installWindowIcon(QStringLiteral("IconTest1"), src);
        QCOMPARE(theme, QStringLiteral("WebApp-IconTest1"));

        const QString dest = Paths::hicolorAppIconPath(QStringLiteral("IconTest1"));
        QVERIFY(QFile::exists(dest));
        const QFileDevice::Permissions perms = QFile::permissions(dest);
        QVERIFY(perms & QFileDevice::ReadOwner);
        QVERIFY(perms & QFileDevice::ReadGroup);
        QVERIFY(perms & QFileDevice::ReadOther);
    }

    void create_webapp_writes_theme_icon_in_desktop()
    {
        QImage img(48, 48, QImage::Format_ARGB32);
        img.fill(Qt::green);
        const QString src =
            m_home.homePath() + QStringLiteral("/notion-icon.png");
        QVERIFY(img.save(src, "PNG"));

        Browser browser;
        browser.family = BrowserFamily::Chromium;
        browser.name = TempHome::fakeBrowserName();
        browser.execPath = m_home.fakeChromiumPath();

        WebAppManager manager;
        QVERIFY(manager.createWebApp(
            QStringLiteral("NotionIcon"), {}, QStringLiteral("https://notion.so"),
            src, QStringLiteral("Network"), browser, {}, true, false, false));

        const WebApp app = manager.webApps().constFirst();
        QCOMPARE(app.icon(),
                 Paths::hicolorIconPath(
                     QStringLiteral("chrome-notion.so__-Default")));
        QVERIFY(QFile::exists(Paths::hicolorAppIconPath(app.codename())));
        QVERIFY(QFile::exists(
            Paths::hicolorIconPath(QStringLiteral("chrome-notion.so__-Default"))));

        QFile desktop(app.path());
        QVERIFY(desktop.open(QIODevice::ReadOnly | QIODevice::Text));
        const QString content = QString::fromUtf8(desktop.readAll());
        QVERIFY(content.contains(
            QStringLiteral("Icon=")
            + Paths::hicolorIconPath(
                QStringLiteral("chrome-notion.so__-Default"))));
        QVERIFY(content.contains(
            QStringLiteral("StartupWMClass=chrome-notion.so__-Default")));
        QVERIFY(content.contains(
            QStringLiteral("--class=chrome-notion.so__-Default")));
        QVERIFY(!content.contains(QStringLiteral("CHROME_DESKTOP=")));
        QVERIFY(content.contains(QStringLiteral("--user-data-dir=")));
    }

private:
    TempHome m_home;
};

QTEST_GUILESS_MAIN(TstWindowIcon)
#include "tst_windowicon.moc"
