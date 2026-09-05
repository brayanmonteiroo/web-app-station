// SPDX-License-Identifier: MIT
#pragma once

#include <QString>

class ProfileService
{
public:
    static void ensureFirefoxProfile(const QString &profilePath, bool navbar);
    static void ensureEpiphanyProfile(const QString &profilePath,
                                      const QString &shareLink,
                                      const QString &iconPath);
    static void ensureFalkonProfile(const QString &codename);
    static void deleteProfiles(const QString &codename);
};
