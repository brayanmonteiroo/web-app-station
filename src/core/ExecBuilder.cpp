// SPDX-License-Identifier: MIT
#include "ExecBuilder.h"

#include "Paths.h"
#include "ProfileService.h"

#include <QDir>

QString ExecBuilder::build(const Browser &browser,
                           const QString &codename,
                           const QString &url,
                           const QString &icon,
                           const QString &customParameters,
                           bool isolateProfile,
                           bool navbar,
                           bool privateWindow)
{
    QString exec;

    if (browser.isFirefoxLike()) {
        QString profilesDir = Paths::firefoxProfilesDir();
        switch (browser.family) {
        case BrowserFamily::FirefoxFlatpak:
            profilesDir = Paths::firefoxFlatpakProfilesDir();
            break;
        case BrowserFamily::ZenFlatpak:
            profilesDir = Paths::zenFlatpakProfilesDir();
            break;
        case BrowserFamily::FirefoxSnap:
            profilesDir = Paths::firefoxSnapProfilesDir();
            break;
        case BrowserFamily::LibreWolfFlatpak:
            profilesDir = Paths::librewolfFlatpakProfilesDir();
            break;
        case BrowserFamily::FloorpFlatpak:
            profilesDir = Paths::floorpFlatpakProfilesDir();
            break;
        case BrowserFamily::WaterfoxFlatpak:
            profilesDir = Paths::waterfoxFlatpakProfilesDir();
            break;
        default:
            break;
        }

        const QString profilePath =
            QDir(profilesDir).filePath(codename);
        ProfileService::ensureFirefoxProfile(profilePath, navbar);

        exec = QStringLiteral(
                   "sh -c 'XAPP_FORCE_GTKWINDOW_ICON=\"%1\" %2 "
                   "--class WebApp-%3 --name WebApp-%3 --profile %4 "
                   "--no-remote")
                   .arg(icon, browser.execPath, codename, profilePath);
        if (privateWindow) {
            exec += QStringLiteral(" --private-window");
        }
        if (!customParameters.trimmed().isEmpty()) {
            exec += QLatin1Char(' ') + customParameters.trimmed();
        }
        exec += QStringLiteral(" \"%1\"'").arg(url);
        return exec;
    }

    if (browser.family == BrowserFamily::Epiphany) {
        const QString profilePath =
            QDir(Paths::epiphanyProfilesDir())
                .filePath(QStringLiteral("org.gnome.Epiphany.WebApp-%1")
                              .arg(codename));
        const QString shareLink =
            QDir::homePath()
            + QStringLiteral("/.local/share/org.gnome.Epiphany.WebApp-%1")
                  .arg(codename);
        ProfileService::ensureEpiphanyProfile(profilePath, shareLink, icon);

        exec = QStringLiteral("%1 --application-mode --profile=\"%2\" \"%3\"")
                   .arg(browser.execPath, shareLink, url);
        if (!customParameters.trimmed().isEmpty()) {
            exec += QLatin1Char(' ') + customParameters.trimmed();
        }
        return exec;
    }

    if (browser.family == BrowserFamily::Falkon) {
        ProfileService::ensureFalkonProfile(codename);
        exec = browser.execPath
            + QStringLiteral(" --wmclass=WebApp-%1").arg(codename);
        if (isolateProfile) {
            exec += QStringLiteral(" --profile=") + codename;
        }
        if (privateWindow) {
            exec += QStringLiteral(" --private-browsing");
        }
        if (!customParameters.trimmed().isEmpty()) {
            exec += QLatin1Char(' ') + customParameters.trimmed();
        }
        exec += QStringLiteral(" --no-remote ") + url;
        return exec;
    }

    // Chromium family
    if (isolateProfile) {
        const QString profilePath =
            QDir(Paths::chromiumProfilesDir()).filePath(codename);
        QDir().mkpath(profilePath);
        exec = QStringLiteral(
                   "%1 --app=\"%2\" --class=WebApp-%3 --name=WebApp-%3 "
                   "--user-data-dir=%4")
                   .arg(browser.execPath, url, codename, profilePath);
    } else {
        exec = QStringLiteral(
                   "%1 --app=\"%2\" --class=WebApp-%3 --name=WebApp-%3")
                   .arg(browser.execPath, url, codename);
    }

    if (privateWindow) {
        if (browser.name.startsWith(QStringLiteral("Microsoft Edge"))) {
            exec += QStringLiteral(" --inprivate");
        } else {
            exec += QStringLiteral(" --incognito");
        }
    }
    if (!customParameters.trimmed().isEmpty()) {
        exec += QLatin1Char(' ') + customParameters.trimmed();
    }
    return exec;
}
