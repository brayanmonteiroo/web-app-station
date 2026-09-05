// SPDX-License-Identifier: MIT
#pragma once

#include "WebApp.h"

#include <QString>

namespace DesktopEntry {

[[nodiscard]] WebApp parse(const QString &path, const QString &codename);
[[nodiscard]] bool write(const WebApp &app, const QString &execLine);
[[nodiscard]] bool updateFields(const WebApp &app, const QString &execLine);

} // namespace DesktopEntry
