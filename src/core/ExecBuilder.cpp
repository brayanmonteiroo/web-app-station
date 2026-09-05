// SPDX-License-Identifier: MIT
#include "ExecBuilder.h"

#include "Paths.h"
#include "ProfileService.h"

#include <QDir>
#include <QRegularExpression>
#include <QStringList>
#include <QUrl>

namespace {

QString normalizeCustomParameters(const QString &raw)
{
    static const QStringList known = {
        QStringLiteral("start-maximized"),
        QStringLiteral("start-fullscreen"),
        QStringLiteral("new-window"),
        QStringLiteral("disable-extensions"),
    };

    const QStringList parts =
        raw.trimmed().split(QRegularExpression(QStringLiteral("\\s+")),
                            Qt::SkipEmptyParts);
    QStringList out;
    out.reserve(parts.size());
    for (QString part : parts) {
        if (part.startsWith(QLatin1String("--"))) {
            out.append(part);
            continue;
        }
        // Legado: "-start-maximized" ou "start-maximized"
        QString bare = part;
        if (bare.startsWith(QLatin1Char('-'))) {
            bare = bare.mid(1);
        }
        if (known.contains(bare)) {
            out.append(QLatin1String("--") + bare);
        } else {
            out.append(part);
        }
    }
    return out.join(QLatin1Char(' '));
}

bool hasFlag(const QString &custom, const QString &flag)
{
    const QStringList parts =
        custom.split(QRegularExpression(QStringLiteral("\\s+")),
                     Qt::SkipEmptyParts);
    return parts.contains(flag);
}

/** Flags só do Chromium — Firefox não entende; tratamos no perfil/CLI próprio. */
QString firefoxSafeCustomParameters(const QString &custom)
{
    const QStringList skip = {
        QStringLiteral("--start-maximized"),
        QStringLiteral("--start-fullscreen"),
        QStringLiteral("--disable-extensions"),
    };
    QStringList out;
    for (const QString &part :
         custom.split(QRegularExpression(QStringLiteral("\\s+")),
                      Qt::SkipEmptyParts)) {
        if (skip.contains(part)) {
            continue;
        }
        out.append(part);
    }
    return out.join(QLatin1Char(' '));
}

QString chromiumClassPrefix(const QString &browserName)
{
    const QString n = browserName.toLower();
    if (n.contains(QStringLiteral("brave"))) {
        return QStringLiteral("brave-");
    }
    if (n.contains(QStringLiteral("vivaldi"))) {
        return QStringLiteral("vivaldi-");
    }
    if (n.contains(QStringLiteral("edge"))) {
        return QStringLiteral("msedge-");
    }
    // Chrome, Chromium, Ungoogled, etc. — mesmo prefixo do PWA nativo.
    return QStringLiteral("chrome-");
}

bool browserNameLooksChromium(const QString &browserName)
{
    const QString n = browserName.toLower();
    return n.contains(QStringLiteral("chrome"))
        || n.contains(QStringLiteral("chromium"))
        || n.contains(QStringLiteral("brave"))
        || n.contains(QStringLiteral("vivaldi"))
        || n.contains(QStringLiteral("edge"))
        || n.contains(QStringLiteral("ungoogled"));
}

} // namespace

QString ExecBuilder::chromiumWmClass(const QString &browserName,
                                     const QString &url)
{
    // Paridade com Web App Hub: "{host}/{path}" com '/' → '_'.
    // https://app.notion.com → app.notion.com__ → chrome-app.notion.com__-Default
    const QUrl parsed(url);
    QString host = parsed.host();
    if (host.isEmpty()) {
        host = QStringLiteral("webapp");
    }
    QString path = parsed.path();
    if (path.isEmpty()) {
        path = QStringLiteral("/");
    }
    QString domainPath = host + QLatin1Char('/') + path;
    domainPath.replace(QLatin1Char('/'), QLatin1Char('_'));
    return chromiumClassPrefix(browserName) + domainPath
        + QStringLiteral("-Default");
}

QString ExecBuilder::startupWmClass(const Browser &browser,
                                    const QString &codename,
                                    const QString &url)
{
    return startupWmClass(browser.name, codename, url,
                          browser.family == BrowserFamily::Chromium);
}

QString ExecBuilder::startupWmClass(const QString &browserName,
                                    const QString &codename,
                                    const QString &url,
                                    bool chromiumFamily)
{
    if (chromiumFamily || browserNameLooksChromium(browserName)) {
        return chromiumWmClass(browserName, url);
    }
    return QStringLiteral("WebApp-%1").arg(codename);
}

QString ExecBuilder::build(const Browser &browser,
                           const QString &codename,
                           const QString &url,
                           const QString &icon,
                           const QString &customParameters,
                           bool isolateProfile,
                           bool navbar,
                           bool privateWindow)
{
    const QString custom = normalizeCustomParameters(customParameters);
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
        const bool startMaximized =
            hasFlag(custom, QStringLiteral("--start-maximized"));
        ProfileService::ensureFirefoxProfile(profilePath, navbar,
                                             startMaximized);

        exec = QStringLiteral(
                   "sh -c 'XAPP_FORCE_GTKWINDOW_ICON=\"%1\" %2 "
                   "--class WebApp-%3 --name WebApp-%3 --profile %4 "
                   "--no-remote")
                   .arg(icon, browser.execPath, codename, profilePath);
        if (privateWindow) {
            exec += QStringLiteral(" --private-window");
        }
        if (hasFlag(custom, QStringLiteral("--start-fullscreen"))) {
            exec += QStringLiteral(" --kiosk");
        }
        const QString firefoxCustom = firefoxSafeCustomParameters(custom);
        if (!firefoxCustom.isEmpty()) {
            exec += QLatin1Char(' ') + firefoxCustom;
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
        if (!custom.isEmpty()) {
            exec += QLatin1Char(' ') + custom;
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
        if (!custom.isEmpty()) {
            exec += QLatin1Char(' ') + custom;
        }
        exec += QStringLiteral(" --no-remote ") + url;
        return exec;
    }

    // Chromium: WMClass no formato PWA/Hub (chrome-host__-Default) para o
    // Plasma Wayland associar o ícone. user-data-dir só com perfil isolado.
    const QString wmClass = chromiumWmClass(browser.name, url);

    exec = browser.execPath + QStringLiteral(" --no-first-run");
    if (!custom.isEmpty()) {
        exec += QLatin1Char(' ') + custom;
    }
    exec += QStringLiteral(" --app=\"%1\" --class=%2 --name=%2")
                .arg(url, wmClass);

    if (isolateProfile) {
        const QString profilePath =
            QDir(Paths::chromiumProfilesDir()).filePath(codename);
        QDir().mkpath(profilePath);
        exec += QStringLiteral(" --user-data-dir=%1").arg(profilePath);
    }

    if (privateWindow) {
        if (browser.name.startsWith(QStringLiteral("Microsoft Edge"))) {
            exec += QStringLiteral(" --inprivate");
        } else {
            exec += QStringLiteral(" --incognito");
        }
    }
    return exec;
}
