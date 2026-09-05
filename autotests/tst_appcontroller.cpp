// SPDX-License-Identifier: MIT
#include <KLocalizedString>

#include <QDir>
#include <QFile>
#include <QImage>
#include <QSignalSpy>
#include <QTemporaryFile>
#include <QTest>

#include "AppController.h"
#include "BrowserDetector.h"
#include "FaviconService.h"
#include "Paths.h"
#include "UpdateService.h"

#include "common/TempHome.h"

class TstAppController : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase()
    {
        QVERIFY(m_home.isValid());
        KLocalizedString::setApplicationDomain("webappstation");
        BrowserDetector detector;
        QVERIFY(!detector.installedBrowsers().isEmpty());
    }

    void init()
    {
        // Garante dirs limpos entre testes deste executável.
        const QDir apps(m_home.applicationsDir());
        for (const QString &f :
             apps.entryList({QStringLiteral("WebApp-*.desktop")}, QDir::Files)) {
            QFile::remove(apps.filePath(f));
        }
    }

    void categories_use_freedesktop_ids()
    {
        AppController app;
        const QVariantList cats = app.categories();
        QVERIFY(!cats.isEmpty());
        bool hasNetwork = false;
        for (const QVariant &v : cats) {
            const QVariantMap m = v.toMap();
            QCOMPARE(m.value(QStringLiteral("id")).toString().contains(
                         QStringLiteral("WebApps")),
                     false);
            if (m.value(QStringLiteral("id")).toString()
                == QStringLiteral("Network")) {
                hasNetwork = true;
            }
        }
        QVERIFY(hasNetwork);
    }

    void normalizeUrl_from_qml_api()
    {
        AppController app;
        QCOMPARE(app.normalizeUrl(QStringLiteral("exemplo.com")),
                 QStringLiteral("http://exemplo.com"));
    }

    void create_edit_delete_list_flow()
    {
        AppController app;
        QVERIFY(app.hasBrowsers());
        QCOMPARE(app.webApps()->count(), 0);

        const int chromiumIndex =
            app.browserIndexForName(TempHome::fakeBrowserName());
        QVERIFY(chromiumIndex >= 0);
        // Garante Chromium fake (não o Firefox real do host).
        QCOMPARE(app.browsers().at(chromiumIndex).toMap()
                     .value(QStringLiteral("name"))
                     .toString(),
                 TempHome::fakeBrowserName());

        QImage img(32, 32, QImage::Format_ARGB32);
        img.fill(Qt::darkCyan);
        const QString iconPath =
            m_home.homePath() + QStringLiteral("/payload-icon.png");
        QVERIFY(img.save(iconPath, "PNG"));

        QVariantMap payload;
        payload.insert(QStringLiteral("name"), QStringLiteral("Notion"));
        payload.insert(QStringLiteral("description"), QStringLiteral("Notas"));
        payload.insert(QStringLiteral("url"),
                       QStringLiteral("https://app.notion.com"));
        payload.insert(QStringLiteral("icon"), iconPath);
        payload.insert(QStringLiteral("category"), QStringLiteral("Network"));
        payload.insert(QStringLiteral("browserIndex"), chromiumIndex);
        payload.insert(QStringLiteral("customParameters"),
                       QStringLiteral("--start-maximized"));
        payload.insert(QStringLiteral("isolateProfile"), true);
        payload.insert(QStringLiteral("navbar"), false);
        payload.insert(QStringLiteral("privateWindow"), false);

        QVERIFY(app.createWebApp(payload));
        QCOMPARE(app.webApps()->count(), 1);
        QCOMPARE(app.webAppAt(0).value(QStringLiteral("name")).toString(),
                 QStringLiteral("Notion"));

        QVariantMap edit = payload;
        edit.insert(QStringLiteral("isolateProfile"), false);
        edit.insert(QStringLiteral("category"), QStringLiteral("Utility"));
        QVERIFY(app.editWebApp(0, edit));

        const QVariantMap after = app.webAppAt(0);
        QCOMPARE(after.value(QStringLiteral("isolateProfile")).toBool(), false);
        QCOMPARE(after.value(QStringLiteral("category")).toString(),
                 QStringLiteral("Utility"));
        QCOMPARE(after.value(QStringLiteral("customParameters")).toString(),
                 QStringLiteral("--start-maximized"));

        // Confirma Exec regenerado sem aspas/corrupção.
        QFile desktop(after.value(QStringLiteral("path")).toString());
        QVERIFY(desktop.open(QIODevice::ReadOnly | QIODevice::Text));
        const QString desktopText = QString::fromUtf8(desktop.readAll());
        QVERIFY(desktopText.contains(QStringLiteral("Categories=Utility;")));
        QVERIFY(desktopText.contains(QStringLiteral("X-WebApp-Isolated=false")));
        QVERIFY(!desktopText.contains(QStringLiteral("Exec=\"")));
        QVERIFY(desktopText.contains(QStringLiteral("--start-maximized")));
        // isolate=false → sem user-data-dir (compartilha sessão do Chrome).
        QVERIFY(!desktopText.contains(QStringLiteral("--user-data-dir=")));
        QVERIFY(desktopText.contains(
            QStringLiteral("StartupWMClass=chrome-app.notion.com__-Default")));
        QVERIFY(desktopText.contains(
            QStringLiteral("--class=chrome-app.notion.com__-Default")));
        QVERIFY(!desktopText.contains(QStringLiteral("CHROME_DESKTOP=")));
        QVERIFY(desktopText.contains(
            QStringLiteral("Icon=")
            + Paths::hicolorIconPath(
                QStringLiteral("chrome-app.notion.com__-Default"))));
        QVERIFY(desktopText.contains(QStringLiteral("X-WebApp-Browser=")
                                     + TempHome::fakeBrowserName()));

        QVERIFY(app.launchWebApp(0));
        QVERIFY(app.deleteWebApp(0));
        QCOMPARE(app.webApps()->count(), 0);
    }

    void favicon_guess_theme_icon()
    {
        FaviconService fav;
        QCOMPARE(fav.guessThemeIcon(QStringLiteral("https://www.youtube.com")),
                 QStringLiteral("web-google-youtube"));
        QCOMPARE(fav.guessThemeIcon(QStringLiteral("https://mail.google.com")),
                 QStringLiteral("web-google-gmail"));
        QCOMPARE(fav.guessThemeIcon(QStringLiteral("https://notion.so")),
                 QStringLiteral("web-notion"));
    }

    void favicon_persist_icon()
    {
        FaviconService fav;
        QTemporaryFile tmp(m_home.homePath()
                           + QStringLiteral("/iconXXXXXX.png"));
        QVERIFY(tmp.open());
        QImage img(16, 16, QImage::Format_ARGB32);
        img.fill(Qt::red);
        QVERIFY(img.save(tmp.fileName(), "PNG"));
        tmp.close();

        const QString dest =
            fav.persistIcon(tmp.fileName(), QStringLiteral("Notion"));
        QVERIFY(dest.endsWith(QStringLiteral("/Notion.png")));
        QVERIFY(QFile::exists(dest));
    }

    void update_pref_roundTrip()
    {
        UpdateService updates;
        QCOMPARE(updates.autoUpdatePref(), QStringLiteral("notasked"));
        updates.savePref(true);
        QCOMPARE(updates.autoUpdatePref(), QStringLiteral("enabled"));
        updates.savePref(false);
        QCOMPARE(updates.autoUpdatePref(), QStringLiteral("disabled"));
        QVERIFY(QFile::exists(m_home.configDir()
                              + QStringLiteral("/update.conf")));
    }

    void update_without_appimage_emits_failure_on_manual()
    {
        qunsetenv("APPIMAGE");
        UpdateService updates;
        QSignalSpy spy(&updates, &UpdateService::updateFailed);
        updates.checkAndApply(true);
        QCOMPARE(spy.count(), 1);
    }

private:
    TempHome m_home;
};

QTEST_GUILESS_MAIN(TstAppController)
#include "tst_appcontroller.moc"
