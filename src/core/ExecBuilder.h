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
};
