// SPDX-License-Identifier: MIT
#pragma once

#include "Browser.h"
#include "BrowserDetector.h"
#include "WebApp.h"

#include <QObject>
#include <QVariantList>

class WebAppManager : public QObject
{
    Q_OBJECT
public:
    explicit WebAppManager(QObject *parent = nullptr);

    [[nodiscard]] QList<WebApp> webApps() const;
    [[nodiscard]] QList<Browser> installedBrowsers() const;

    bool createWebApp(const QString &name,
                      const QString &description,
                      const QString &url,
                      const QString &icon,
                      const QString &category,
                      const Browser &browser,
                      const QString &customParameters,
                      bool isolateProfile,
                      bool navbar,
                      bool privateWindow);

    bool editWebApp(const WebApp &existing,
                    const QString &name,
                    const QString &description,
                    const QString &url,
                    const QString &icon,
                    const QString &category,
                    const Browser &browser,
                    const QString &customParameters,
                    bool isolateProfile,
                    bool navbar,
                    bool privateWindow);

    bool deleteWebApp(const WebApp &app);
    bool launchWebApp(const WebApp &app);

    [[nodiscard]] static QString normalizeUrl(const QString &url);
    [[nodiscard]] static QString makeCodename(const QString &name);

private:
    BrowserDetector m_detector;
};
