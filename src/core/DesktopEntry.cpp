// SPDX-License-Identifier: MIT
#include "DesktopEntry.h"

#include "ExecBuilder.h"

#include <QFile>
#include <QRegularExpression>
#include <QSaveFile>
#include <QTextStream>

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
            || line.contains(QStringLiteral("StartupWMClass=ICE-SSB"))
            || line.contains(QStringLiteral("StartupWMClass=chrome-"))
            || line.contains(QStringLiteral("StartupWMClass=brave-"))
            || line.contains(QStringLiteral("StartupWMClass=vivaldi-"))
            || line.contains(QStringLiteral("StartupWMClass=msedge-"))) {
            isWebApp = true;
            continue;
        }
        if (line.startsWith(QStringLiteral("X-WebApp-Browser="))
            || line.startsWith(QStringLiteral("X-WebApp-URL="))) {
            isWebApp = true;
        }
        if (line.startsWith(QStringLiteral("Name="))) {
            app.setName(line.mid(5));
        } else if (line.startsWith(QStringLiteral("Comment="))) {
            QString desc = line.mid(8);
            if (desc == QStringLiteral("Web App")
                || desc == QStringLiteral("Aplicativo web")) {
                desc.clear();
            }
            app.setDescription(desc);
        } else if (line.startsWith(QStringLiteral("Icon="))) {
            app.setIcon(line.mid(5));
        } else if (line.startsWith(QStringLiteral("Exec="))) {
            QString exec = line.mid(5);
            // Repara Exec corrompido por QSettings legado (aspas externas).
            if (exec.size() >= 2 && exec.startsWith(QLatin1Char('"'))
                && exec.endsWith(QLatin1Char('"'))) {
                exec = exec.mid(1, exec.size() - 2);
                exec.replace(QStringLiteral("\\\""), QStringLiteral("\""));
            }
            app.setExec(exec);
        } else if (line.startsWith(QStringLiteral("Categories="))) {
            // Aceita Main Categories do freedesktop (ex.: Network;Utility;).
            // Remove prefixo legado GTK; e mapeia WebApps → Network.
            QString categories = line.mid(11);
            if (categories.size() >= 2 && categories.startsWith(QLatin1Char('"'))
                && categories.endsWith(QLatin1Char('"'))) {
                categories = categories.mid(1, categories.size() - 2);
            }
            categories.replace(QStringLiteral("GTK;"), QString());
            const QStringList parts =
                categories.split(QLatin1Char(';'), Qt::SkipEmptyParts);
            QString category = parts.isEmpty() ? QStringLiteral("Network")
                                               : parts.constFirst();
            if (category == QStringLiteral("WebApps")
                || category == QStringLiteral("Web")) {
                category = QStringLiteral("Network");
            }
            app.setCategory(category);
        } else if (line.startsWith(QStringLiteral("X-WebApp-Browser="))) {
            app.setBrowserName(line.mid(17));
        } else if (line.startsWith(QStringLiteral("X-WebApp-URL="))) {
            app.setUrl(line.mid(13));
        } else if (line.startsWith(
                       QStringLiteral("X-WebApp-CustomParameters="))) {
            QString params =
                line.mid(QStringLiteral("X-WebApp-CustomParameters=").size());
            // Legado QSettings / mid errado: "start-maximized" ou "-start-maximized"
            const QStringList parts =
                params.trimmed().split(QRegularExpression(QStringLiteral("\\s+")),
                                       Qt::SkipEmptyParts);
            QStringList fixed;
            for (QString p : parts) {
                if (p == QStringLiteral("start-maximized")
                    || p == QStringLiteral("-start-maximized")) {
                    p = QStringLiteral("--start-maximized");
                } else if (p == QStringLiteral("start-fullscreen")
                           || p == QStringLiteral("-start-fullscreen")) {
                    p = QStringLiteral("--start-fullscreen");
                } else if (p == QStringLiteral("new-window")
                           || p == QStringLiteral("-new-window")) {
                    p = QStringLiteral("--new-window");
                } else if (p == QStringLiteral("disable-extensions")
                           || p == QStringLiteral("-disable-extensions")) {
                    p = QStringLiteral("--disable-extensions");
                }
                fixed.append(p);
            }
            app.setCustomParameters(fixed.join(QLatin1Char(' ')));
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
        desc = QStringLiteral("Aplicativo web");
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
        << "Categories=" << app.category() << ";\n"
        << "MimeType=text/html;text/xml;application/xhtml_xml;\n"
        << "StartupWMClass="
        << ExecBuilder::startupWmClass(app.browserName(), app.codename(),
                                       app.url(),
                                       /*chromiumFamily=*/false)
        << '\n'
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
    // Nunca usar QSettings em .desktop: corrompe Exec (aspas), Categories
    // ([Desktop%20Entry], ; como comentário) e remove "--" dos parâmetros.
    return write(app, execLine);
}
