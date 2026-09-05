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
    app.setIcon(icon.isEmpty() ? QStringLiteral("org.kde.webappstation")
                               : icon);
    app.setCategory(category.isEmpty() ? QStringLiteral("WebApps") : category);
    app.setBrowserName(browser.name);
    app.setCustomParameters(customParameters);
    app.setIsolateProfile(isolateProfile);
    app.setNavbar(navbar);
    app.setPrivateWindow(privateWindow);

    const QString exec = ExecBuilder::build(browser, codename, app.url(),
                                            app.icon(), customParameters,
                                            isolateProfile, navbar,
                                            privateWindow);
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
    app.setIcon(icon);
    app.setCategory(category);
    app.setBrowserName(browser.name);
    app.setCustomParameters(customParameters);
    app.setIsolateProfile(isolateProfile);
    app.setNavbar(navbar);
    app.setPrivateWindow(privateWindow);

    const QString exec =
        ExecBuilder::build(browser, app.codename(), app.url(), app.icon(),
                           customParameters, isolateProfile, navbar,
                           privateWindow);
    return DesktopEntry::updateFields(app, exec);
}

bool WebAppManager::deleteWebApp(const WebApp &app)
{
    ProfileService::deleteProfiles(app.codename());
    if (QFileInfo::exists(app.path())) {
        QFile::remove(app.path());
    }
    return true;
}

bool WebAppManager::launchWebApp(const WebApp &app)
{
    if (app.exec().isEmpty()) {
        return false;
    }
    return QProcess::startDetached(QStringLiteral("/bin/sh"),
                                   {QStringLiteral("-c"), app.exec()});
}
