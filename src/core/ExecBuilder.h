// SPDX-License-Identifier: MIT
#pragma once

#include "Browser.h"

#include <QString>

class ExecBuilder
{
public:
    [[nodiscard]] static QString build(const Browser &browser,
                                       const QString &codename,
                                       const QString &url,
                                       const QString &icon,
                                       const QString &customParameters,
                                       bool isolateProfile,
                                       bool navbar,
                                       bool privateWindow);

    /**
     * WMClass / Wayland app_id no estilo Chrome PWA / Web App Hub:
     * chrome-{host}/{path}-Default com '/' → '_'
     * (ex.: https://app.notion.com → chrome-app.notion.com__-Default).
     */
    [[nodiscard]] static QString chromiumWmClass(const QString &browserName,
                                                 const QString &url);

    /** StartupWMClass do .desktop (Chromium = formato Hub; demais = WebApp-codename). */
    [[nodiscard]] static QString startupWmClass(const Browser &browser,
                                                const QString &codename,
                                                const QString &url);

    [[nodiscard]] static QString startupWmClass(const QString &browserName,
                                                const QString &codename,
                                                const QString &url,
                                                bool chromiumFamily);
};
