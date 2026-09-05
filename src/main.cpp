// SPDX-License-Identifier: MIT
#include "AppController.h"

#include <KAboutData>
#include <KLocalizedQmlContext>
#include <KLocalizedString>

#include <QApplication>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    KLocalizedString::setApplicationDomain("webappstation");

    KAboutData about(
        QStringLiteral("webappstation"),
        i18n("Web App Station"),
        QStringLiteral(WEBAPPSTATION_VERSION),
        i18n("Execute sites como se fossem aplicativos"),
        KAboutLicense::MIT,
        i18n("© 2026 Brayan Monteiro"));
    about.addAuthor(QStringLiteral("Brayan Monteiro"), QString(),
                    QStringLiteral("https://github.com/brayanmonteiroo"));
    about.setHomepage(
        QStringLiteral("https://github.com/brayanmonteiroo/web-app-station"));
    KAboutData::setApplicationData(about);
    QApplication::setWindowIcon(QIcon::fromTheme(
        QStringLiteral("org.kde.webappstation"),
        QIcon::fromTheme(QStringLiteral("applications-internet"))));

    QQuickStyle::setStyle(QStringLiteral("org.kde.desktop"));

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
