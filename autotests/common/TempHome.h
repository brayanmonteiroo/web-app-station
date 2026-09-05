// SPDX-License-Identifier: MIT
#pragma once

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>

/**
 * Isola HOME / XDG_* e instala um Chromium Flatpak falso detectável
 * por BrowserDetector (path relativo a $HOME).
 */
class TempHome
{
public:
    TempHome()
    {
        Q_ASSERT(m_dir.isValid());

        const QString home = m_dir.path();
        qputenv("HOME", QFile::encodeName(home));
        qputenv("XDG_DATA_HOME",
                QFile::encodeName(home + QStringLiteral("/.local/share")));
        qputenv("XDG_CONFIG_HOME",
                QFile::encodeName(home + QStringLiteral("/.config")));
        qputenv("WEBAPPSTATION_CONFIG_DIR",
                QFile::encodeName(home + QStringLiteral("/config")));
        qputenv("XDG_CACHE_HOME",
                QFile::encodeName(home + QStringLiteral("/.cache")));

        QDir().mkpath(applicationsDir());
        QDir().mkpath(iconsDir());
        QDir().mkpath(configDir());
        installFakeChromium();
    }

    [[nodiscard]] bool isValid() const { return m_dir.isValid(); }

    [[nodiscard]] QString homePath() const { return m_dir.path(); }

    [[nodiscard]] QString applicationsDir() const
    {
        return m_dir.path() + QStringLiteral("/.local/share/applications");
    }

    [[nodiscard]] QString iconsDir() const
    {
        return m_dir.path()
            + QStringLiteral("/.local/share/web-app-station/icons");
    }

    [[nodiscard]] QString configDir() const
    {
        return m_dir.path() + QStringLiteral("/config");
    }

    [[nodiscard]] QString fakeChromiumPath() const
    {
        return m_dir.path()
            + QStringLiteral(
                "/.local/share/flatpak/exports/bin/org.chromium.Chromium");
    }

    [[nodiscard]] static QString fakeBrowserName()
    {
        return QStringLiteral("Chromium (Flatpak)");
    }

private:
    void installFakeChromium()
    {
        const QString path = fakeChromiumPath();
        QDir().mkpath(QFileInfo(path).absolutePath());
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            return;
        }
        file.write("#!/bin/sh\nexit 0\n");
        file.close();
        file.setPermissions(path,
                            QFileDevice::ReadOwner | QFileDevice::WriteOwner
                                | QFileDevice::ExeOwner
                                | QFileDevice::ReadGroup
                                | QFileDevice::ExeGroup
                                | QFileDevice::ReadOther
                                | QFileDevice::ExeOther);
    }

    QTemporaryDir m_dir;
};
