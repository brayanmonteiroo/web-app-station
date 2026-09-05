// SPDX-License-Identifier: MIT
#include "WebAppManager.h"

#include "DesktopEntry.h"
#include "ExecBuilder.h"
#include "Paths.h"
#include "ProfileService.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QRandomGenerator>

#include <algorithm>

namespace {

bool isChromiumWmIconName(const QString &icon)
{
    const QString base = QFileInfo(icon).completeBaseName();
    const QString name = base.isEmpty() ? icon : base;
    return name.startsWith(QStringLiteral("chrome-"))
        || name.startsWith(QStringLiteral("brave-"))
        || name.startsWith(QStringLiteral("vivaldi-"))
        || name.startsWith(QStringLiteral("msedge-"));
}

QString chromiumIconThemeName(const QString &icon)
{
    if (icon.contains(QLatin1Char('/'))) {
        return QFileInfo(icon).completeBaseName();
    }
    return icon;
}

/** Icon= deve coincidir com o app_id Wayland (canto da janela no Plasma). */
QString finalizeDesktopIcon(const Browser &browser,
                            const QString &codename,
                            const QString &url,
                            const QString &webAppThemeIcon)
{
    if (browser.family != BrowserFamily::Chromium) {
        return webAppThemeIcon;
    }
    const QString wmClass =
        ExecBuilder::chromiumWmClass(browser.name, url);
    const QString source = Paths::hicolorAppIconPath(codename);
    if (QFileInfo::exists(source)) {
        ProfileService::installIconThemeAlias(wmClass, source);
    }
    // Path absoluto (como Web App Hub) + nome = app_id no hicolor.
    const QString absolute = Paths::hicolorIconPath(wmClass);
    return QFileInfo::exists(absolute) ? absolute : wmClass;
}

} // namespace

WebAppManager::WebAppManager(QObject *parent)
    : QObject(parent)
{
    Paths::ensureUserDirs();
}

QString WebAppManager::normalizeUrl(const QString &url)
{
    QString trimmed = url.trimmed();
    if (trimmed.isEmpty()) {
        return {};
    }
    if (!trimmed.contains(QStringLiteral("://"))) {
        trimmed.prepend(QStringLiteral("http://"));
    }
    return trimmed;
}

QString WebAppManager::makeCodename(const QString &name)
{
    QString alpha;
    for (QChar c : name) {
        if (c.isLetter()) {
            alpha.append(c);
        }
    }
    if (alpha.isEmpty()) {
        alpha = QStringLiteral("WebApp");
    }
    const int code = QRandomGenerator::global()->bounded(1000, 9999);
    return alpha + QString::number(code);
}

QList<WebApp> WebAppManager::webApps() const
{
    QList<WebApp> apps;
    const QDir appsDir(Paths::applicationsDir());
    const auto entries = appsDir.entryList({QStringLiteral("WebApp-*.desktop"),
                                            QStringLiteral("webapp-*.desktop")},
                                           QDir::Files);
    for (const QString &fileName : entries) {
        QString codename = fileName;
        codename.remove(QStringLiteral(".desktop"), Qt::CaseInsensitive);
        if (codename.startsWith(QStringLiteral("WebApp-"), Qt::CaseInsensitive)) {
            codename = codename.mid(7);
        } else if (codename.startsWith(QStringLiteral("webapp-"),
                                       Qt::CaseInsensitive)) {
            codename = codename.mid(7);
        }
        WebApp app =
            DesktopEntry::parse(appsDir.filePath(fileName), codename);
        if (app.isValid()) {
            apps.append(app);
        }
    }
    std::sort(apps.begin(), apps.end(),
              [](const WebApp &a, const WebApp &b) {
                  return a.name().localeAwareCompare(b.name()) < 0;
              });
    return apps;
}

QList<Browser> WebAppManager::installedBrowsers() const
{
    return m_detector.installedBrowsers();
}

bool WebAppManager::createWebApp(const QString &name,
                                 const QString &description,
                                 const QString &url,
                                 const QString &icon,
                                 const QString &category,
                                 const Browser &browser,
                                 const QString &customParameters,
                                 bool isolateProfile,
                                 bool navbar,
                                 bool privateWindow)
{
    Paths::ensureUserDirs();
    const QString codename = makeCodename(name);
    const QString path =
        QDir(Paths::applicationsDir())
            .filePath(QStringLiteral("WebApp-%1.desktop").arg(codename));

    WebApp app;
    app.setPath(path);
    app.setCodename(codename);
    app.setName(name);
    app.setDescription(description);
    app.setUrl(normalizeUrl(url));
    const QString themeIcon =
        ProfileService::installWindowIcon(codename, icon);
    app.setIcon(finalizeDesktopIcon(
        browser, codename, app.url(),
        themeIcon.isEmpty() ? QStringLiteral("org.kde.webappstation")
                            : themeIcon));
    app.setCategory(category.isEmpty() ? QStringLiteral("Network") : category);
    app.setBrowserName(browser.name);
    app.setCustomParameters(customParameters);
    app.setIsolateProfile(isolateProfile);
    app.setNavbar(navbar);
    app.setPrivateWindow(privateWindow);

    // Ícone com path absoluto para Firefox (XAPP_FORCE_GTKWINDOW_ICON).
    const QString iconForExec = Paths::hicolorAppIconPath(codename);
    const QString exec = ExecBuilder::build(
        browser, codename, app.url(),
        QFileInfo::exists(iconForExec) ? iconForExec : app.icon(),
        customParameters, isolateProfile, navbar, privateWindow);
    if (!DesktopEntry::write(app, exec)) {
        return false;
    }

    if (browser.family == BrowserFamily::Epiphany) {
        const QString profilePath =
            QDir(Paths::epiphanyProfilesDir())
                .filePath(QStringLiteral("org.gnome.Epiphany.WebApp-%1")
                              .arg(codename));
        QDir().mkpath(profilePath);
        const QString newPath =
            QDir(profilePath)
                .filePath(QStringLiteral("org.gnome.Epiphany.WebApp-%1.desktop")
                              .arg(codename));
        QFile::rename(path, newPath);
        QFile::link(newPath, path);
    }

    return true;
}

bool WebAppManager::editWebApp(const WebApp &existing,
                               const QString &name,
                               const QString &description,
                               const QString &url,
                               const QString &icon,
                               const QString &category,
                               const Browser &browser,
                               const QString &customParameters,
                               bool isolateProfile,
                               bool navbar,
                               bool privateWindow)
{
    WebApp app = existing;
    app.setName(name);
    app.setDescription(description);
    app.setUrl(normalizeUrl(url));
    if (isChromiumWmIconName(existing.icon())
        && chromiumIconThemeName(existing.icon())
            != ExecBuilder::chromiumWmClass(browser.name, app.url())) {
        ProfileService::removeThemeIcon(chromiumIconThemeName(existing.icon()));
    }
    const QString themeIcon =
        ProfileService::installWindowIcon(existing.codename(), icon);
    app.setIcon(finalizeDesktopIcon(
        browser, existing.codename(), app.url(),
        themeIcon.isEmpty() ? icon : themeIcon));
    app.setCategory(category);
    app.setBrowserName(browser.name);
    app.setCustomParameters(customParameters);
    app.setIsolateProfile(isolateProfile);
    app.setNavbar(navbar);
    app.setPrivateWindow(privateWindow);

    const QString iconForExec = Paths::hicolorAppIconPath(existing.codename());
    const QString exec = ExecBuilder::build(
        browser, app.codename(), app.url(),
        QFileInfo::exists(iconForExec) ? iconForExec : app.icon(),
        customParameters, isolateProfile, navbar, privateWindow);
    return DesktopEntry::updateFields(app, exec);
}

bool WebAppManager::deleteWebApp(const WebApp &app)
{
    if (isChromiumWmIconName(app.icon())) {
        ProfileService::removeThemeIcon(chromiumIconThemeName(app.icon()));
    }
    ProfileService::deleteProfiles(app.codename());
    if (QFileInfo::exists(app.path())) {
        QFile::remove(app.path());
    }
    return true;
}

bool WebAppManager::launchWebApp(const WebApp &app)
{
    // Regenera o Exec a partir dos metadados — evita falha se o .desktop
    // tiver sido corrompido por edições antigas (QSettings).
    Browser browser = m_detector.findByName(app.browserName());
    QString exec = app.exec();
    if (!browser.name.isEmpty()) {
        const QString iconForExec = Paths::hicolorAppIconPath(app.codename());
        exec = ExecBuilder::build(
            browser, app.codename(), app.url(),
            QFileInfo::exists(iconForExec) ? iconForExec : app.icon(),
            app.customParameters(), app.isolateProfile(), app.navbar(),
            app.privateWindow());
        WebApp fresh = app;
        // Mantém Icon= alinhado ao app_id no Chromium (canto da janela).
        fresh.setIcon(finalizeDesktopIcon(
            browser, app.codename(), app.url(),
            Paths::windowIconThemeName(app.codename())));
        (void)DesktopEntry::write(fresh, exec);
    }
    if (exec.isEmpty()) {
        return false;
    }

    // Preferir gio launch: o Plasma associa a janela ao .desktop (ícone).
    if (QFileInfo::exists(app.path())
        && QProcess::startDetached(QStringLiteral("gio"),
                                   {QStringLiteral("launch"), app.path()})) {
        return true;
    }

    return QProcess::startDetached(QStringLiteral("/bin/sh"),
                                   {QStringLiteral("-c"), exec});
}
