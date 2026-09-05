// SPDX-License-Identifier: MIT
#include "ProfileService.h"

#include "Paths.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>

void ProfileService::ensureFirefoxProfile(const QString &profilePath,
                                          bool navbar)
{
    QDir().mkpath(profilePath);
    const QString templateDir = Paths::firefoxProfileTemplateDir();
    if (QDir(templateDir).exists()) {
        QDir src(templateDir);
        const auto entries =
            src.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
        for (const QFileInfo &info : entries) {
            const QString dest = QDir(profilePath).filePath(info.fileName());
            if (info.isDir()) {
                QDir().mkpath(dest);
            } else if (!QFileInfo::exists(dest)) {
                QFile::copy(info.absoluteFilePath(), dest);
            }
        }
        QDir().mkpath(QDir(profilePath).filePath(QStringLiteral("chrome")));
    } else {
        QDir().mkpath(QDir(profilePath).filePath(QStringLiteral("chrome")));
        const QString userJs =
            QDir(profilePath).filePath(QStringLiteral("user.js"));
        if (!QFileInfo::exists(userJs)) {
            QFile f(userJs);
            if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
                f.write(
                    "// Web App Station Firefox profile\n"
                    "user_pref(\"toolkit.legacyUserProfileCustomizations.stylesheets\", true);\n"
                    "user_pref(\"browser.tabs.warnOnClose\", false);\n");
            }
        }
    }

    const QString chromeCss =
        QDir(profilePath).filePath(QStringLiteral("chrome/userChrome.css"));
    if (navbar) {
        const QString srcCss = Paths::firefoxNavbarCssPath();
        if (QFileInfo::exists(srcCss)) {
            QFile::remove(chromeCss);
            QFile::copy(srcCss, chromeCss);
        } else {
            QFile f(chromeCss);
            if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
                f.write("/* navbar enabled */\n");
            }
        }
    } else {
        QFile f(chromeCss);
        if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
            f.write(
                "#nav-bar, #TabsToolbar, #PersonalToolbar { visibility: "
                "collapse !important; }\n");
        }
    }
}

void ProfileService::ensureEpiphanyProfile(const QString &profilePath,
                                           const QString &shareLink,
                                           const QString &iconPath)
{
    QDir().mkpath(profilePath);
    QFileInfo linkInfo(shareLink);
    if (!linkInfo.exists()) {
        QFile::link(profilePath, shareLink);
    }
    if (!iconPath.isEmpty() && QFileInfo::exists(iconPath)) {
        const QString dest =
            QDir(profilePath).filePath(QStringLiteral("app-icon.png"));
        QFile::remove(dest);
        QFile::copy(iconPath, dest);
    }
    const QString appMode =
        QDir(profilePath).filePath(QStringLiteral(".app"));
    if (!QFileInfo::exists(appMode)) {
        QFile f(appMode);
        if (!f.open(QIODevice::WriteOnly)) {
            return;
        }
    }
}

void ProfileService::ensureFalkonProfile(const QString &codename)
{
    const QString profilePath =
        QDir(Paths::falkonProfilesDir()).filePath(codename);
    QDir().mkpath(profilePath);
    const QString link =
        QDir::homePath()
        + QStringLiteral("/.config/falkon/profiles/") + codename;
    QDir().mkpath(QFileInfo(link).absolutePath());
    if (!QFileInfo::exists(link)) {
        QFile::link(profilePath, link);
    }
}

void ProfileService::deleteProfiles(const QString &codename)
{
    auto wipe = [](const QString &path) {
        QDir dir(path);
        if (dir.exists()) {
            dir.removeRecursively();
        }
    };

    wipe(QDir(Paths::firefoxProfilesDir()).filePath(codename));
    wipe(QDir(Paths::firefoxFlatpakProfilesDir()).filePath(codename));
    wipe(QDir(Paths::zenFlatpakProfilesDir()).filePath(codename));
    wipe(QDir(Paths::firefoxSnapProfilesDir()).filePath(codename));
    wipe(QDir(Paths::chromiumProfilesDir()).filePath(codename));
    wipe(QDir(Paths::librewolfFlatpakProfilesDir()).filePath(codename));
    wipe(QDir(Paths::floorpFlatpakProfilesDir()).filePath(codename));

    const QString epi =
        QDir(Paths::epiphanyProfilesDir())
            .filePath(QStringLiteral("org.gnome.Epiphany.WebApp-%1")
                          .arg(codename));
    wipe(epi);
    const QString epiLink =
        QDir::homePath()
        + QStringLiteral("/.local/share/org.gnome.Epiphany.WebApp-%1")
              .arg(codename);
    QFile::remove(epiLink);

    const QString falkon =
        QDir(Paths::falkonProfilesDir()).filePath(codename);
    wipe(falkon);
    const QString falkonLink =
        QDir::homePath()
        + QStringLiteral("/.config/falkon/profiles/") + codename;
    QFile::remove(falkonLink);
}
