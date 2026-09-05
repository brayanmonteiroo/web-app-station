// SPDX-License-Identifier: MIT
#pragma once

#include "Browser.h"

#include <QList>
#include <QObject>

class BrowserDetector : public QObject
{
    Q_OBJECT
public:
    explicit BrowserDetector(QObject *parent = nullptr);

    [[nodiscard]] QList<Browser> installedBrowsers() const;
    [[nodiscard]] QList<Browser> allSupportedBrowsers() const;
    [[nodiscard]] Browser findByName(const QString &name) const;
};
