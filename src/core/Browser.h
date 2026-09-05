// SPDX-License-Identifier: MIT
#pragma once

#include <QString>
#include <QMetaType>

enum class BrowserFamily {
    Firefox,
    FirefoxFlatpak,
    FirefoxSnap,
    LibreWolfFlatpak,
    WaterfoxFlatpak,
    FloorpFlatpak,
    ZenFlatpak,
    Chromium,
    Epiphany,
    Falkon
};

struct Browser {
    BrowserFamily family = BrowserFamily::Chromium;
    QString name;
    QString execPath;
    QString testPath;

    [[nodiscard]] bool isFirefoxLike() const
    {
        switch (family) {
        case BrowserFamily::Firefox:
        case BrowserFamily::FirefoxFlatpak:
        case BrowserFamily::FirefoxSnap:
        case BrowserFamily::LibreWolfFlatpak:
        case BrowserFamily::WaterfoxFlatpak:
        case BrowserFamily::FloorpFlatpak:
        case BrowserFamily::ZenFlatpak:
            return true;
        default:
            return false;
        }
    }

    [[nodiscard]] bool supportsIsolationToggle() const
    {
        return family == BrowserFamily::Chromium
            || family == BrowserFamily::Falkon;
    }

    [[nodiscard]] bool supportsNavbar() const
    {
        return isFirefoxLike();
    }
};

Q_DECLARE_METATYPE(Browser)
