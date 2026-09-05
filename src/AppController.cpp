// SPDX-License-Identifier: MIT
#include "AppController.h"

#include <KLocalizedString>

WebAppListModel::WebAppListModel(WebAppManager *manager, QObject *parent)
    : QAbstractListModel(parent)
    , m_manager(manager)
{
    reload();
}

int WebAppListModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return m_apps.size();
}

QVariant WebAppListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_apps.size()) {
        return {};
    }
    const WebApp &app = m_apps.at(index.row());
    switch (role) {
    case NameRole:
        return app.name();
    case BrowserRole:
        return app.browserName();
    case IconRole:
        return app.icon();
    case UrlRole:
        return app.url();
    case PathRole:
        return app.path();
    case CodenameRole:
        return app.codename();
    case DescriptionRole:
        return app.description();
    case CategoryRole:
        return app.category();
    case CustomParametersRole:
        return app.customParameters();
    case IsolateRole:
        return app.isolateProfile();
    case NavbarRole:
        return app.navbar();
    case PrivateRole:
        return app.privateWindow();
    case ExecRole:
        return app.exec();
    default:
        return {};
    }
}

QHash<int, QByteArray> WebAppListModel::roleNames() const
{
    return {
        {NameRole, QByteArrayLiteral("name")},
        {BrowserRole, QByteArrayLiteral("browser")},
        {IconRole, QByteArrayLiteral("icon")},
        {UrlRole, QByteArrayLiteral("url")},
        {PathRole, QByteArrayLiteral("path")},
        {CodenameRole, QByteArrayLiteral("codename")},
        {DescriptionRole, QByteArrayLiteral("description")},
        {CategoryRole, QByteArrayLiteral("category")},
        {CustomParametersRole, QByteArrayLiteral("customParameters")},
        {IsolateRole, QByteArrayLiteral("isolateProfile")},
        {NavbarRole, QByteArrayLiteral("navbar")},
        {PrivateRole, QByteArrayLiteral("privateWindow")},
        {ExecRole, QByteArrayLiteral("execLine")},
    };
}

void WebAppListModel::reload()
{
    beginResetModel();
    m_apps = m_manager->webApps();
    endResetModel();
    Q_EMIT countChanged();
}

WebApp WebAppListModel::at(int row) const
{
    if (row < 0 || row >= m_apps.size()) {
        return {};
    }
    return m_apps.at(row);
}

AppController::AppController(QObject *parent)
    : QObject(parent)
    , m_model(new WebAppListModel(&m_manager, this))
    , m_favicon(new FaviconService(this))
    , m_update(new UpdateService(this))
    , m_locale(new LocaleService(this))
{
    reloadBrowsers();
}

QVariantList AppController::categories() const
{
    // IDs = Main Categories do freedesktop (evita "Achados e perdidos" no KDE).
    // Textos-fonte em pt_BR; inglês via po/en_GB (KI18n ignora po/en).
    return {
        QVariantMap{{QStringLiteral("id"), QStringLiteral("Network")},
                    {QStringLiteral("name"), i18n("Internet")}},
        QVariantMap{{QStringLiteral("id"), QStringLiteral("Utility")},
                    {QStringLiteral("name"), i18n("Acessórios")}},
        QVariantMap{{QStringLiteral("id"), QStringLiteral("Game")},
                    {QStringLiteral("name"), i18n("Jogos")}},
        QVariantMap{{QStringLiteral("id"), QStringLiteral("Graphics")},
                    {QStringLiteral("name"), i18n("Gráficos")}},
        QVariantMap{{QStringLiteral("id"), QStringLiteral("Office")},
                    {QStringLiteral("name"), i18n("Escritório")}},
        QVariantMap{{QStringLiteral("id"), QStringLiteral("AudioVideo")},
                    {QStringLiteral("name"), i18n("Som e vídeo")}},
        QVariantMap{{QStringLiteral("id"), QStringLiteral("Development")},
                    {QStringLiteral("name"), i18n("Programação")}},
        QVariantMap{{QStringLiteral("id"), QStringLiteral("Education")},
                    {QStringLiteral("name"), i18n("Educação")}},
    };
}

void AppController::reloadBrowsers()
{
    m_browserObjects = m_manager.installedBrowsers();
    m_browsers.clear();
    for (int i = 0; i < m_browserObjects.size(); ++i) {
        const Browser &b = m_browserObjects.at(i);
        m_browsers.append(QVariantMap{
            {QStringLiteral("index"), i},
            {QStringLiteral("name"), b.name},
            {QStringLiteral("supportsIsolation"), b.supportsIsolationToggle()},
            {QStringLiteral("supportsNavbar"), b.supportsNavbar()},
            {QStringLiteral("firefoxLike"), b.isFirefoxLike()},
        });
    }
    Q_EMIT browsersChanged();
}

void AppController::setStatus(const QString &message)
{
    if (m_statusMessage == message) {
        return;
    }
    m_statusMessage = message;
    Q_EMIT statusMessageChanged();
}

Browser AppController::browserFromIndex(int index) const
{
    if (index < 0 || index >= m_browserObjects.size()) {
        return {};
    }
    return m_browserObjects.at(index);
}

QString AppController::normalizeUrl(const QString &url) const
{
    return WebAppManager::normalizeUrl(url);
}

int AppController::browserIndexForName(const QString &name) const
{
    for (int i = 0; i < m_browserObjects.size(); ++i) {
        if (m_browserObjects.at(i).name == name) {
            return i;
        }
    }
    return 0;
}

void AppController::refresh()
{
    reloadBrowsers();
    m_model->reload();
}

bool AppController::createWebApp(const QVariantMap &data)
{
    const Browser browser =
        browserFromIndex(data.value(QStringLiteral("browserIndex")).toInt());
    if (browser.name.isEmpty()) {
        setStatus(QStringLiteral("Nenhum navegador selecionado."));
        return false;
    }

    QString icon = data.value(QStringLiteral("icon")).toString();
    if (icon.contains(QStringLiteral("/tmp"))) {
        icon = m_favicon->persistIcon(
            icon, data.value(QStringLiteral("name")).toString());
    }

    const bool ok = m_manager.createWebApp(
        data.value(QStringLiteral("name")).toString(),
        data.value(QStringLiteral("description")).toString(),
        data.value(QStringLiteral("url")).toString(),
        icon,
        data.value(QStringLiteral("category")).toString(),
        browser,
        data.value(QStringLiteral("customParameters")).toString(),
        data.value(QStringLiteral("isolateProfile"), true).toBool(),
        data.value(QStringLiteral("navbar"), false).toBool(),
        data.value(QStringLiteral("privateWindow"), false).toBool());

    if (ok) {
        m_model->reload();
        setStatus(QStringLiteral("Aplicativo web criado."));
    } else {
        setStatus(QStringLiteral("Falha ao criar aplicativo web."));
    }
    return ok;
}

bool AppController::editWebApp(int index, const QVariantMap &data)
{
    const WebApp existing = m_model->at(index);
    if (!existing.isValid()) {
        return false;
    }

    Browser browser = m_manager.installedBrowsers().isEmpty()
        ? Browser{}
        : browserFromIndex(browserIndexForName(existing.browserName()));
    if (browser.name.isEmpty()) {
        // Keep exec builder working with stored browser name if possible
        browser.name = existing.browserName();
        browser.execPath = existing.browserName().toLower();
        browser.family = BrowserFamily::Chromium;
        for (const Browser &b : m_manager.installedBrowsers()) {
            if (b.name == existing.browserName()) {
                browser = b;
                break;
            }
        }
    }

    QString icon = data.value(QStringLiteral("icon")).toString();
    if (icon.contains(QStringLiteral("/tmp"))) {
        icon = m_favicon->persistIcon(
            icon, data.value(QStringLiteral("name")).toString());
    }

    const bool ok = m_manager.editWebApp(
        existing,
        data.value(QStringLiteral("name")).toString(),
        data.value(QStringLiteral("description")).toString(),
        data.value(QStringLiteral("url")).toString(),
        icon,
        data.value(QStringLiteral("category")).toString(),
        browser,
        data.value(QStringLiteral("customParameters")).toString(),
        data.value(QStringLiteral("isolateProfile"), true).toBool(),
        data.value(QStringLiteral("navbar"), false).toBool(),
        data.value(QStringLiteral("privateWindow"), false).toBool());

    if (ok) {
        m_model->reload();
        setStatus(QStringLiteral("Aplicativo web atualizado."));
    } else {
        setStatus(QStringLiteral("Falha ao editar aplicativo web."));
    }
    return ok;
}

bool AppController::deleteWebApp(int index)
{
    const WebApp app = m_model->at(index);
    if (!app.isValid()) {
        return false;
    }
    const bool ok = m_manager.deleteWebApp(app);
    m_model->reload();
    setStatus(ok ? QStringLiteral("Aplicativo web removido.")
                 : QStringLiteral("Falha ao remover aplicativo web."));
    return ok;
}

bool AppController::launchWebApp(int index)
{
    const WebApp app = m_model->at(index);
    if (!app.isValid()) {
        return false;
    }
    return m_manager.launchWebApp(app);
}

QVariantMap AppController::webAppAt(int index) const
{
    const WebApp app = m_model->at(index);
    if (!app.isValid()) {
        return {};
    }
    return {
        {QStringLiteral("name"), app.name()},
        {QStringLiteral("description"), app.description()},
        {QStringLiteral("url"), app.url()},
        {QStringLiteral("icon"), app.icon()},
        {QStringLiteral("category"), app.category()},
        {QStringLiteral("browser"), app.browserName()},
        {QStringLiteral("customParameters"), app.customParameters()},
        {QStringLiteral("isolateProfile"), app.isolateProfile()},
        {QStringLiteral("navbar"), app.navbar()},
        {QStringLiteral("privateWindow"), app.privateWindow()},
        {QStringLiteral("codename"), app.codename()},
        {QStringLiteral("path"), app.path()},
    };
}
