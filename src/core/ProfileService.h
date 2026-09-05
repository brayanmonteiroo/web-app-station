// SPDX-License-Identifier: MIT
#pragma once

#include <QString>

class ProfileService
{
public:
    static void ensureFirefoxProfile(const QString &profilePath,
                                     bool navbar,
                                     bool startMaximized = false);
    /** Copia o ícone para hicolor como WebApp-{codename}.png. */
    [[nodiscard]] static QString installWindowIcon(const QString &codename,
                                                   const QString &sourceIcon);
    /**
     * Copia/instala um segundo nome no hicolor (ex. chrome-host__-Default.png)
     * para o canto da janela no Plasma Wayland (lookup por app_id).
     */
    static bool installIconThemeAlias(const QString &themeName,
                                      const QString &sourcePngPath);
    static void removeThemeIcon(const QString &themeName);
    static void ensureEpiphanyProfile(const QString &profilePath,
                                      const QString &shareLink,
                                      const QString &iconPath);
    static void ensureFalkonProfile(const QString &codename);
    static void deleteProfiles(const QString &codename);
};
