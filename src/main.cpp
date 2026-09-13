// SPDX-License-Identifier: MIT
#include "AppController.h"
#include "LocaleService.h"

#include <KAboutData>
#include <KLocalizedQmlContext>
#include <KLocalizedString>

#include <QApplication>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QStandardPaths>

namespace {

QStringList iconCandidatePaths()
{
    QStringList roots;
    if (qEnvironmentVariableIsSet("APPDIR")) {
        roots << QDir(qEnvironmentVariable("APPDIR")).filePath(
            QStringLiteral("usr/share/icons"));
    }
    roots << QDir::cleanPath(
        QDir(QCoreApplication::applicationDirPath())
            .filePath(QStringLiteral("../share/icons")));

    QStringList out;
    const QStringList relative = {
        QStringLiteral("hicolor/256x256/apps/org.kde.webappstation.png"),
        QStringLiteral("hicolor/48x48/apps/org.kde.webappstation.png"),
        QStringLiteral("hicolor/scalable/apps/org.kde.webappstation.svg"),
    };
    for (const QString &root : roots) {
        for (const QString &rel : relative) {
            out << QDir(root).filePath(rel);
        }
    }
    out << QStandardPaths::locate(
        QStandardPaths::GenericDataLocation,
        QStringLiteral(
            "icons/hicolor/256x256/apps/org.kde.webappstation.png"));
    out << QStandardPaths::locate(
        QStandardPaths::GenericDataLocation,
        QStringLiteral(
            "icons/hicolor/scalable/apps/org.kde.webappstation.svg"));
    out.removeAll(QString());
    return out;
}

QString bundledAppIconPath()
{
    // Preferir PNG: evita dlopen de libQt6Svg no boot (SEGV se ELF corrompido).
    for (const QString &path : iconCandidatePaths()) {
        if (QFile::exists(path)) {
            return path;
        }
    }
    return {};
}

} // namespace

int main(int argc, char *argv[])
{
    // Preferência de idioma antes do domínio KI18n / QApplication i18n.
    LocaleService::applyFromConfigEarly();

    QApplication app(argc, argv);
    KLocalizedString::setApplicationDomain("webappstation");
    LocaleService::finishI18nSetup();

    KAboutData about(
        QStringLiteral("webappstation"),
        i18n("Estação de Aplicativos Web"),
        QStringLiteral(WEBAPPSTATION_VERSION),
        i18n("Execute sites como se fossem aplicativos"),
        KAboutLicense::MIT,
        i18n("© 2026 Brayan Monteiro"));
    about.addAuthor(QStringLiteral("Brayan Monteiro"), QString(),
                    QStringLiteral("https://github.com/brayanmonteiroo"));
    about.setHomepage(
        QStringLiteral("https://github.com/brayanmonteiroo/web-app-station"));
    KAboutData::setApplicationData(about);

    // Estilo único DE-agnóstico (sem org.kde.desktop / Plasma).
    QQuickStyle::setStyle(QStringLiteral("Fusion"));

    const QString iconPath = bundledAppIconPath();
    if (!iconPath.isEmpty()) {
        QApplication::setWindowIcon(QIcon(iconPath));
    } else {
        QApplication::setWindowIcon(QIcon::fromTheme(
            QStringLiteral("org.kde.webappstation"),
            QIcon::fromTheme(QStringLiteral("applications-internet"))));
    }

    QQmlApplicationEngine engine;
    auto *controller = new AppController(&app);
    KLocalization::setupLocalizedContext(&engine);
    engine.rootContext()->setContextProperty(QStringLiteral("App"), controller);

    engine.loadFromModule("org.kde.webappstation", "Main");
    if (engine.rootObjects().isEmpty()) {
        return -1;
    }

    return app.exec();
}
