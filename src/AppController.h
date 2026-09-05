// SPDX-License-Identifier: MIT
#pragma once

#include "FaviconService.h"
#include "LocaleService.h"
#include "UpdateService.h"
#include "WebAppManager.h"

#include <QAbstractListModel>
#include <QObject>
#include <QVariantList>

class WebAppListModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)
public:
    enum Roles {
        NameRole = Qt::UserRole + 1,
        BrowserRole,
        IconRole,
        UrlRole,
        PathRole,
        CodenameRole,
        DescriptionRole,
        CategoryRole,
        CustomParametersRole,
        IsolateRole,
        NavbarRole,
        PrivateRole,
        ExecRole
    };

    explicit WebAppListModel(WebAppManager *manager, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE void reload();
    [[nodiscard]] WebApp at(int row) const;
    [[nodiscard]] int count() const { return m_apps.size(); }

Q_SIGNALS:
    void countChanged();

private:
    WebAppManager *m_manager = nullptr;
    QList<WebApp> m_apps;
};

class AppController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(WebAppListModel *webApps READ webApps CONSTANT)
    Q_PROPERTY(FaviconService *faviconService READ faviconService CONSTANT)
    Q_PROPERTY(UpdateService *updateService READ updateService CONSTANT)
    Q_PROPERTY(LocaleService *localeService READ localeService CONSTANT)
    Q_PROPERTY(QString version READ version CONSTANT)
    Q_PROPERTY(QVariantList browsers READ browsers NOTIFY browsersChanged)
    Q_PROPERTY(QVariantList categories READ categories CONSTANT)
    Q_PROPERTY(bool hasBrowsers READ hasBrowsers NOTIFY browsersChanged)
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusMessageChanged)

public:
    explicit AppController(QObject *parent = nullptr);

    WebAppListModel *webApps() const { return m_model; }
    FaviconService *faviconService() const { return m_favicon; }
    UpdateService *updateService() const { return m_update; }
    LocaleService *localeService() const { return m_locale; }
    QString version() const
    {
        return QStringLiteral(WEBAPPSTATION_VERSION);
    }
    QVariantList browsers() const { return m_browsers; }
    QVariantList categories() const;
    bool hasBrowsers() const { return !m_browsers.isEmpty(); }
    QString statusMessage() const { return m_statusMessage; }

    Q_INVOKABLE bool createWebApp(const QVariantMap &data);
    Q_INVOKABLE bool editWebApp(int index, const QVariantMap &data);
    Q_INVOKABLE bool deleteWebApp(int index);
    Q_INVOKABLE bool launchWebApp(int index);
    Q_INVOKABLE QVariantMap webAppAt(int index) const;
    Q_INVOKABLE void refresh();
    Q_INVOKABLE QString normalizeUrl(const QString &url) const;
    Q_INVOKABLE int browserIndexForName(const QString &name) const;

Q_SIGNALS:
    void browsersChanged();
    void statusMessageChanged();

private:
    void reloadBrowsers();
    void setStatus(const QString &message);
    Browser browserFromIndex(int index) const;

    WebAppManager m_manager;
    WebAppListModel *m_model = nullptr;
    FaviconService *m_favicon = nullptr;
    UpdateService *m_update = nullptr;
    LocaleService *m_locale = nullptr;
    QVariantList m_browsers;
    QList<Browser> m_browserObjects;
    QString m_statusMessage;
};
