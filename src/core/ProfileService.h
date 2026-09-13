// SPDX-License-Identifier: MIT
#pragma once

#include <QString>

class ProfileService
{
public:
    static void ensureFirefoxProfile(const QString &profilePath,
                                     bool navbar,
                                     bool startMaximized = false);
    /**
     * Garante diretório de perfil Chromium isolado e, se startMaximized,
     * grava Default/Preferences com janela maximizada.
     */
    static void ensureChromiumProfile(const QString &profilePath,
                                      bool startMaximized = false);

    /**
     * Diretório user-data do browser (perfil compartilhado), ex.
     * ~/.config/google-chrome ou Flatpak equivalente.
     */
    [[nodiscard]] static QString chromiumSharedUserDataDir(
        const QString &browserName,
        const QString &execPath);

    /** Merge maximized=true em {userDataDir}/Default/Preferences. */
    static void ensureChromiumMaximized(const QString &userDataDir);

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
