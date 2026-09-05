// SPDX-License-Identifier: MIT
#include "DesktopEntry.h"

#include <QFile>
#include <QSaveFile>
#include <QTextStream>
#include <QSettings>

namespace {

QString boolToString(bool value)
{
    return value ? QStringLiteral("true") : QStringLiteral("false");
}

} // namespace

WebApp DesktopEntry::parse(const QString &path, const QString &codename)
{
    WebApp app;
    app.setPath(path);
    app.setCodename(codename);

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return app;
    }

    bool isWebApp = false;
    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.contains(QStringLiteral("StartupWMClass=WebApp"))
            || line.contains(QStringLiteral("StartupWMClass=Chromium"))
            || line.contains(QStringLiteral("StartupWMClass=ICE-SSB"))) {
            isWebApp = true;
            continue;
        }
        if (line.startsWith(QStringLiteral("Name="))) {
            app.setName(line.mid(5));
        } else if (line.startsWith(QStringLiteral("Comment="))) {
            QString desc = line.mid(8);
            if (desc == QStringLiteral("Web App")) {
                desc.clear();
            }
            app.setDescription(desc);
        } else if (line.startsWith(QStringLiteral("Icon="))) {
            app.setIcon(line.mid(5));
        } else if (line.startsWith(QStringLiteral("Exec="))) {
            app.setExec(line.mid(5));
        } else if (line.startsWith(QStringLiteral("Categories="))) {
            QString category = line.mid(11);
            category.replace(QStringLiteral("GTK;"), QString());
            category.remove(QLatin1Char(';'));
            app.setCategory(category);
        } else if (line.startsWith(QStringLiteral("X-WebApp-Browser="))) {
            app.setBrowserName(line.mid(17));
        } else if (line.startsWith(QStringLiteral("X-WebApp-URL="))) {
            app.setUrl(line.mid(13));
        } else if (line.startsWith(
                       QStringLiteral("X-WebApp-CustomParameters="))) {
            app.setCustomParameters(line.mid(27));
        } else if (line.startsWith(QStringLiteral("X-WebApp-Isolated="))) {
            app.setIsolateProfile(
                line.mid(18).compare(QStringLiteral("true"),
                                     Qt::CaseInsensitive)
                == 0);
        } else if (line.startsWith(QStringLiteral("X-WebApp-Navbar="))) {
            app.setNavbar(line.mid(16).compare(QStringLiteral("true"),
                                               Qt::CaseInsensitive)
                          == 0);
        } else if (line.startsWith(QStringLiteral("X-WebApp-PrivateWindow="))) {
            app.setPrivateWindow(
                line.mid(23).compare(QStringLiteral("true"),
                                     Qt::CaseInsensitive)
                == 0);
        }
    }

    if (isWebApp && !app.name().isEmpty() && !app.icon().isEmpty()) {
        app.setValid(true);
    }
    return app;
}

bool DesktopEntry::write(const WebApp &app, const QString &execLine)
{
    QSaveFile file(app.path());
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    QString desc = app.description().trimmed();
    if (desc.isEmpty()) {
        desc = QStringLiteral("Web App");
    }

    QTextStream out(&file);
    out << "[Desktop Entry]\n"
        << "Version=1.0\n"
        << "Name=" << app.name() << '\n'
        << "Comment=" << desc << '\n'
        << "Exec=" << execLine << '\n'
        << "Terminal=false\n"
        << "X-MultipleArgs=false\n"
        << "Type=Application\n"
        << "Icon=" << app.icon() << '\n'
        << "Categories=GTK;" << app.category() << ";\n"
        << "MimeType=text/html;text/xml;application/xhtml_xml;\n"
        << "StartupWMClass=WebApp-" << app.codename() << '\n'
        << "StartupNotify=true\n"
        << "X-WebApp-Browser=" << app.browserName() << '\n'
        << "X-WebApp-URL=" << app.url() << '\n'
        << "X-WebApp-CustomParameters=" << app.customParameters() << '\n'
        << "X-WebApp-Navbar=" << boolToString(app.navbar()) << '\n'
        << "X-WebApp-PrivateWindow=" << boolToString(app.privateWindow())
        << '\n'
        << "X-WebApp-Isolated=" << boolToString(app.isolateProfile()) << '\n';

    return file.commit();
}

bool DesktopEntry::updateFields(const WebApp &app, const QString &execLine)
{
    QSettings settings(app.path(), QSettings::IniFormat);
    settings.beginGroup(QStringLiteral("Desktop Entry"));

    QString desc = app.description().trimmed();
    if (desc.isEmpty()) {
        desc = QStringLiteral("Web App");
    }

    settings.setValue(QStringLiteral("Name"), app.name());
    settings.setValue(QStringLiteral("Icon"), app.icon());
    settings.setValue(QStringLiteral("Comment"), desc);
    settings.setValue(QStringLiteral("Categories"),
                      QStringLiteral("GTK;%1;").arg(app.category()));
    settings.setValue(QStringLiteral("Exec"), execLine);
    settings.setValue(QStringLiteral("X-WebApp-Browser"), app.browserName());
    settings.setValue(QStringLiteral("X-WebApp-URL"), app.url());
    settings.setValue(QStringLiteral("X-WebApp-CustomParameters"),
                      app.customParameters());
    settings.setValue(QStringLiteral("X-WebApp-Isolated"),
                      boolToString(app.isolateProfile()));
    settings.setValue(QStringLiteral("X-WebApp-Navbar"),
                      boolToString(app.navbar()));
    settings.setValue(QStringLiteral("X-WebApp-PrivateWindow"),
                      boolToString(app.privateWindow()));
    settings.endGroup();
    settings.sync();
    return settings.status() == QSettings::NoError;
}
