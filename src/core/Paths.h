// SPDX-License-Identifier: MIT
#pragma once

#include <QString>
#include <QStandardPaths>
#include <QDir>
#include <QCoreApplication>

namespace Paths {

inline QString iceRoot()
{
    return QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)
        + QStringLiteral("/web-app-station");
}

inline QString applicationsDir()
{
    return QStandardPaths::writableLocation(QStandardPaths::ApplicationsLocation);
}

inline QString iconsDir()
{
    return iceRoot() + QStringLiteral("/icons");
}

inline QString chromiumProfilesDir()
{
    return iceRoot() + QStringLiteral("/profiles");
}

inline QString firefoxProfilesDir()
{
    return iceRoot() + QStringLiteral("/firefox");
}

inline QString epiphanyProfilesDir()
{
    return iceRoot() + QStringLiteral("/epiphany");
}

inline QString falkonProfilesDir()
{
    return iceRoot() + QStringLiteral("/falkon");
}

inline QString firefoxFlatpakProfilesDir()
{
    return QDir::homePath()
        + QStringLiteral("/.var/app/org.mozilla.firefox/data/ice/firefox");
}

inline QString firefoxSnapProfilesDir()
{
    return QDir::homePath() + QStringLiteral("/snap/firefox/common/.mozilla/firefox");
}

inline QString zenFlatpakProfilesDir()
{
    return QDir::homePath()
        + QStringLiteral("/.var/app/app.zen_browser.zen/data/ice/zen");
}

inline QString librewolfFlatpakProfilesDir()
{
    return QDir::homePath()
        + QStringLiteral(
            "/.var/app/io.gitlab.librewolf-community/data/ice/librewolf");
}

inline QString waterfoxFlatpakProfilesDir()
{
    return QDir::homePath()
        + QStringLiteral("/.var/app/net.waterfox.waterfox/data");
}

inline QString floorpFlatpakProfilesDir()
{
    return QDir::homePath()
        + QStringLiteral("/.var/app/one.ablaze.floorp/data");
}

/** Read-only resources shipped with the app / AppImage. */
inline QString bundledDataDir()
{
    const QString appDir = QCoreApplication::applicationDirPath();
    // Installed: .../bin → ../share/webappstation
    const QString installed = QDir(appDir).absoluteFilePath(
        QStringLiteral("../share/webappstation"));
    if (QDir(installed).exists()) {
        return installed;
    }
    // Dev / AppImage fallback next to binary
    const QString local = QDir(appDir).absoluteFilePath(QStringLiteral("data"));
    if (QDir(local).exists()) {
        return local;
    }
    return installed;
}

inline QString firefoxProfileTemplateDir()
{
    return bundledDataDir() + QStringLiteral("/firefox/profile");
}

inline QString firefoxNavbarCssPath()
{
    return bundledDataDir()
        + QStringLiteral("/firefox/userChrome-with-navbar.css");
}

inline void ensureUserDirs()
{
    for (const QString &dir :
         {iceRoot(),
          applicationsDir(),
          iconsDir(),
          chromiumProfilesDir(),
          firefoxProfilesDir(),
          epiphanyProfilesDir(),
          falkonProfilesDir()}) {
        QDir().mkpath(dir);
    }
}

} // namespace Paths
