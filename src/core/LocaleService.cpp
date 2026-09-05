// SPDX-License-Identifier: MIT
#include "LocaleService.h"

#include <KLocalizedString>

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QStandardPaths>
#include <QTextStream>

#include <clocale>

namespace {

QString normalizeCode(const QString &raw)
{
    const QString code = raw.trimmed();
    if (code == QStringLiteral("pt_BR") || code == QStringLiteral("pt-BR")
        || code == QStringLiteral("pt")) {
        return QStringLiteral("pt_BR");
    }
    if (code == QStringLiteral("en") || code == QStringLiteral("en_US")
        || code == QStringLiteral("en-US") || code == QStringLiteral("en_GB")
        || code == QStringLiteral("en-GB")) {
        return QStringLiteral("en");
    }
    if (code == QStringLiteral("system") || code.isEmpty()) {
        return QStringLiteral("system");
    }
    return QStringLiteral("system");
}

bool hasCatalog(const QString &localeRoot, const QString &lang)
{
    return QFile::exists(QDir(localeRoot).filePath(
        lang + QStringLiteral("/LC_MESSAGES/webappstation.mo")));
}

} // namespace

LocaleService::LocaleService(QObject *parent)
    : QObject(parent)
{
}

void LocaleService::applyFromConfigEarly()
{
    applyLanguageEnv(loadLanguageCode());
}

void LocaleService::finishI18nSetup()
{
    registerLocaleDirs();
    applyLanguageOverride(loadLanguageCode());
}

QString LocaleService::configPath()
{
    if (qEnvironmentVariableIsSet("WEBAPPSTATION_CONFIG_DIR")) {
        return QDir(qEnvironmentVariable("WEBAPPSTATION_CONFIG_DIR"))
            .filePath(QStringLiteral("ui.conf"));
    }
    return QDir(QStandardPaths::writableLocation(QStandardPaths::ConfigLocation))
        .filePath(QStringLiteral("webappstation/ui.conf"));
}

QString LocaleService::loadLanguageCode()
{
    QFile file(configPath());
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QStringLiteral("system");
    }
    QTextStream in(&file);
    while (!in.atEnd()) {
        const QString line = in.readLine().trimmed();
        if (line.startsWith(QStringLiteral("language="))) {
            return normalizeCode(line.mid(9));
        }
    }
    return QStringLiteral("system");
}

void LocaleService::saveLanguageCode(const QString &code)
{
    const QString path = configPath();
    QDir().mkpath(QFileInfo(path).absolutePath());
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return;
    }
    QTextStream out(&file);
    out << "language=" << normalizeCode(code) << '\n';
}

void LocaleService::applyLanguageEnv(const QString &code)
{
    const QString normalized = normalizeCode(code);
    if (normalized == QStringLiteral("pt_BR")) {
        qputenv("LANGUAGE", "pt_BR");
        qputenv("LANG", "pt_BR.UTF-8");
    } else if (normalized == QStringLiteral("en")) {
        // KI18n trata en/en_US como idioma-fonte (devolve msgid).
        // Catálogo real: po/en_GB → locale/en_GB/.
        qputenv("LANGUAGE", "en_GB");
        qputenv("LANG", "en_GB.UTF-8");
    } else {
        return;
    }
    // Com LANG=C.UTF-8 (containers CI), só qputenv não basta para o gettext.
    setlocale(LC_ALL, "");
}

void LocaleService::registerLocaleDirs()
{
    QStringList roots;
    if (qEnvironmentVariableIsSet("APPDIR")) {
        roots << QDir(qEnvironmentVariable("APPDIR"))
                     .filePath(QStringLiteral("usr/share/locale"));
    }
    if (qEnvironmentVariableIsSet("WEBAPPSTATION_LOCALE_DIR")) {
        roots << qEnvironmentVariable("WEBAPPSTATION_LOCALE_DIR");
    }

    const QString appDir = QCoreApplication::applicationDirPath();
    // Árvore de build (ki18n gera build/locale/…)
    roots << QDir(appDir).filePath(QStringLiteral("../locale"));
    // Prefixo instalado ao lado do binário
    roots << QDir(appDir).filePath(QStringLiteral("../share/locale"));

    for (const QString &root : roots) {
        const QString clean = QDir::cleanPath(root);
        if (!QDir(clean).exists()) {
            continue;
        }
        if (!hasCatalog(clean, QStringLiteral("en_GB"))) {
            continue;
        }
        KLocalizedString::addDomainLocaleDir(QByteArrayLiteral("webappstation"),
                                             clean);
    }
}

void LocaleService::applyLanguageOverride(const QString &code)
{
    const QString normalized = normalizeCode(code);
    applyLanguageEnv(normalized);
    if (normalized == QStringLiteral("en")) {
        // Nunca usar "en"/"en_US" aqui — KI18n ignora o .mo e devolve msgid.
        KLocalizedString::setLanguages({QStringLiteral("en_GB")});
        return;
    }
    if (normalized == QStringLiteral("pt_BR")) {
        // Sem .mo de pt: KI18n devolve o msgid (português fonte).
        KLocalizedString::setLanguages(
            {QStringLiteral("pt_BR"), QStringLiteral("pt")});
        return;
    }
    KLocalizedString::clearLanguages();
}

QString LocaleService::language() const
{
    return loadLanguageCode();
}

void LocaleService::setLanguage(const QString &code)
{
    const QString normalized = normalizeCode(code);
    if (normalized == language()) {
        return;
    }
    saveLanguageCode(normalized);
    Q_EMIT languageChanged();
    Q_EMIT restartRequired();
}

void LocaleService::restartApp()
{
    const QString appImage = QString::fromLocal8Bit(qgetenv("APPIMAGE"));
    const QString program = appImage.isEmpty()
        ? QCoreApplication::applicationFilePath()
        : appImage;

    QStringList args;
    if (appImage.isEmpty()) {
        const QStringList all = QCoreApplication::arguments();
        if (all.size() > 1) {
            args = all.mid(1);
        }
    }

    if (!QProcess::startDetached(program, args)) {
        Q_EMIT restartFailed(
            i18n("Não foi possível reiniciar a Estação de Aplicativos Web."));
        return;
    }
    QCoreApplication::quit();
}
