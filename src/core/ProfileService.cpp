// SPDX-License-Identifier: MIT
#include "ProfileService.h"

#include "Paths.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QTextStream>

namespace {

void writeFirefoxUserJs(const QString &profilePath)
{
    const QString userJs =
        QDir(profilePath).filePath(QStringLiteral("user.js"));
    QFile f(userJs);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        return;
    }
    QTextStream out(&f);
    out << "// Web App Station Firefox profile defaults\n"
        << "user_pref(\"browser.tabs.inTitlebar\", 0);\n"
        << "user_pref(\"toolkit.legacyUserProfileCustomizations.stylesheets\", true);\n"
        << "user_pref(\"browser.tabs.warnOnClose\", false);\n"
        << "user_pref(\"browser.shell.checkDefaultBrowser\", false);\n"
        << "user_pref(\"browser.sessionstore.resume_from_crash\", false);\n";
}

void writeFirefoxXulStore(const QString &profilePath, bool startMaximized)
{
    if (!startMaximized) {
        return;
    }
    const QString path =
        QDir(profilePath).filePath(QStringLiteral("xulstore.json"));
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        return;
    }
    // Firefox ignora --start-maximized (flag do Chromium). Persistimos o
    // sizemode no perfil, como as SSB costumam fazer.
    f.write(
        "{\n"
        "  \"chrome://browser/content/browser.xhtml\": {\n"
        "    \"main-window\": {\n"
        "      \"sizemode\": \"maximized\"\n"
        "    }\n"
        "  },\n"
        "  \"chrome://browser/content/browser.xul\": {\n"
        "    \"main-window\": {\n"
        "      \"sizemode\": \"maximized\"\n"
        "    }\n"
        "  }\n"
        "}\n");
}

} // namespace

QString ProfileService::installWindowIcon(const QString &codename,
                                          const QString &sourceIcon)
{
    const QString themeName = Paths::windowIconThemeName(codename);
    if (codename.isEmpty()) {
        return sourceIcon;
    }

    QString source = sourceIcon;
    if (source.isEmpty() || source == QStringLiteral("org.kde.webappstation")) {
        return themeName;
    }

    // Já é nome de tema WebApp-* (reinstalação).
    if (!source.contains(QLatin1Char('/'))
        && source.startsWith(QStringLiteral("WebApp-"))) {
        return source;
    }

    if (!QFileInfo::exists(source)) {
        return sourceIcon;
    }

    const QString dest = Paths::hicolorAppIconPath(codename);
    QDir().mkpath(QFileInfo(dest).absolutePath());
    QFile::remove(dest);

    QImage image(source);
    if (!image.isNull()) {
        if (image.width() != 256 || image.height() != 256) {
            image = image.scaled(256, 256, Qt::KeepAspectRatio,
                                 Qt::SmoothTransformation);
        }
        if (!image.save(dest, "PNG")) {
            QFile::copy(source, dest);
        }
    } else if (!QFile::copy(source, dest)) {
        return sourceIcon;
    }

    QFile::setPermissions(dest,
                          QFileDevice::ReadOwner | QFileDevice::WriteOwner
                              | QFileDevice::ReadGroup | QFileDevice::ReadOther);

    // Cópia legível também no diretório interno (Firefox XAPP_FORCE_GTKWINDOW_ICON).
    Paths::ensureUserDirs();
    const QString iceCopy =
        QDir(Paths::iconsDir()).filePath(themeName + QStringLiteral(".png"));
    QFile::remove(iceCopy);
    QFile::copy(dest, iceCopy);
    QFile::setPermissions(iceCopy,
                          QFileDevice::ReadOwner | QFileDevice::WriteOwner
                              | QFileDevice::ReadGroup | QFileDevice::ReadOther);

    return themeName;
}

bool ProfileService::installIconThemeAlias(const QString &themeName,
                                           const QString &sourcePngPath)
{
    if (themeName.isEmpty() || themeName.contains(QLatin1Char('/'))
        || !QFileInfo::exists(sourcePngPath)) {
        return false;
    }
    const QString dest = Paths::hicolorIconPath(themeName);
    if (QFileInfo(dest).canonicalFilePath()
        == QFileInfo(sourcePngPath).canonicalFilePath()) {
        return true;
    }
    QDir().mkpath(QFileInfo(dest).absolutePath());
    QFile::remove(dest);
    if (!QFile::copy(sourcePngPath, dest)) {
        return false;
    }
    QFile::setPermissions(dest,
                          QFileDevice::ReadOwner | QFileDevice::WriteOwner
                              | QFileDevice::ReadGroup | QFileDevice::ReadOther);
    return true;
}

void ProfileService::removeThemeIcon(const QString &themeName)
{
    if (themeName.isEmpty() || themeName.contains(QLatin1Char('/'))) {
        return;
    }
    // Não apaga o WebApp-* canônico aqui — deleteProfiles cuida disso.
    if (themeName.startsWith(QStringLiteral("WebApp-"))) {
        return;
    }
    QFile::remove(Paths::hicolorIconPath(themeName));
}

void ProfileService::ensureFirefoxProfile(const QString &profilePath,
                                          bool navbar,
                                          bool startMaximized)
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
                // Copia chrome/userChrome.css do template se ainda não existir
                // (será sobrescrito abaixo conforme navbar).
                if (info.fileName() == QStringLiteral("chrome")) {
                    QDir chromeSrc(info.absoluteFilePath());
                    for (const QFileInfo &c :
                         chromeSrc.entryInfoList(QDir::Files)) {
                        const QString cdest =
                            QDir(dest).filePath(c.fileName());
                        if (!QFileInfo::exists(cdest)) {
                            QFile::copy(c.absoluteFilePath(), cdest);
                        }
                    }
                }
            } else if (!QFileInfo::exists(dest)
                       && info.fileName() != QStringLiteral("user.js")) {
                QFile::copy(info.absoluteFilePath(), dest);
            }
        }
        QDir().mkpath(QDir(profilePath).filePath(QStringLiteral("chrome")));
    } else {
        QDir().mkpath(QDir(profilePath).filePath(QStringLiteral("chrome")));
    }

    // Sempre regrava: perfis antigos precisam do inTitlebar=0.
    writeFirefoxUserJs(profilePath);
    writeFirefoxXulStore(profilePath, startMaximized);

    const QString chromeCss =
        QDir(profilePath).filePath(QStringLiteral("chrome/userChrome.css"));
    if (navbar) {
        const QString srcCss = Paths::firefoxNavbarCssPath();
        if (QFileInfo::exists(srcCss)) {
            QFile::remove(chromeCss);
            QFile::copy(srcCss, chromeCss);
        } else {
            QFile f(chromeCss);
            if (f.open(QIODevice::WriteOnly | QIODevice::Text
                       | QIODevice::Truncate)) {
                f.write(
                    "#nav-bar, #TabsToolbar, #PersonalToolbar { visibility: "
                    "visible !important; }\n");
            }
        }
    } else {
        QFile f(chromeCss);
        if (f.open(QIODevice::WriteOnly | QIODevice::Text
                   | QIODevice::Truncate)) {
            f.write(
                "/* Moldura nativa via browser.tabs.inTitlebar=0 */\n"
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

    QFile::remove(Paths::hicolorAppIconPath(codename));
    QFile::remove(QDir(Paths::iconsDir())
                      .filePath(Paths::windowIconThemeName(codename)
                                + QStringLiteral(".png")));

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
