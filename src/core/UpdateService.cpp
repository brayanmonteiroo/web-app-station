// SPDX-License-Identifier: MIT
#include "UpdateService.h"

#include <KLocalizedString>

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFutureWatcher>
#include <QProcess>
#include <QProcessEnvironment>
#include <QStandardPaths>
#include <QTextStream>
#include <QtConcurrent>

namespace {

QStringList updaterCandidates()
{
    QStringList tools;
    if (qEnvironmentVariableIsSet("APPDIR")) {
        tools << QDir(qEnvironmentVariable("APPDIR"))
                     .filePath(QStringLiteral("usr/bin/appimageupdatetool"));
    }
    // Binário ao lado do webappstation no AppDir montado (/tmp/.mount_*/usr/bin).
    tools << (QCoreApplication::applicationDirPath()
              + QStringLiteral("/appimageupdatetool"));
    tools << QStringLiteral("appimageupdatetool")
          << QStringLiteral("AppImageUpdate");
    tools.removeDuplicates();
    return tools;
}

QString resolveUpdaterTool(const QString &candidate)
{
    if (candidate.isEmpty()) {
        return {};
    }
    if (QFileInfo(candidate).isAbsolute()) {
        const QFileInfo fi(candidate);
        if (fi.exists() && fi.isExecutable()) {
            return fi.absoluteFilePath();
        }
        return {};
    }
    return QStandardPaths::findExecutable(candidate);
}

void prepareUpdaterEnvironment(QProcess &proc, const QString &tool)
{
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    const QFileInfo fi(tool);
    if (fi.isAbsolute()) {
        const QString libRoot =
            QDir::cleanPath(fi.absolutePath() + QStringLiteral("/../lib"));
        const QString updaterLib =
            QDir(libRoot).filePath(QStringLiteral("appimageupdate"));
        QStringList libDirs;
        if (QDir(updaterLib).exists()) {
            libDirs << updaterLib;
        }
        if (QDir(libRoot).exists()) {
            libDirs << libRoot;
        }
        if (!libDirs.isEmpty()) {
            const QString existing = env.value(QStringLiteral("LD_LIBRARY_PATH"));
            QString path = libDirs.join(QLatin1Char(':'));
            if (!existing.isEmpty()) {
                path += QLatin1Char(':') + existing;
            }
            env.insert(QStringLiteral("LD_LIBRARY_PATH"), path);
        }
    }
    proc.setProcessEnvironment(env);
}

struct UpdaterRunResult {
    bool started = false;
    bool crashed = false;
    int exitCode = -1;
    QByteArray standardOutput;
    QByteArray standardError;
};

UpdaterRunResult runUpdater(const QString &tool, const QStringList &args)
{
    UpdaterRunResult result;
    QProcess proc;
    prepareUpdaterEnvironment(proc, tool);
    proc.start(tool, args);
    if (!proc.waitForStarted(5000)) {
        return result;
    }
    result.started = true;
    proc.waitForFinished(-1);
    result.crashed = proc.exitStatus() == QProcess::CrashExit;
    result.exitCode = proc.exitCode();
    result.standardOutput = proc.readAllStandardOutput();
    result.standardError = proc.readAllStandardError();
    return result;
}

QString updaterFailureMessage(const UpdaterRunResult &run)
{
    const QString err = QString::fromUtf8(run.standardError).trimmed();
    const QString out = QString::fromUtf8(run.standardOutput).trimmed();
    const QString combined = err.isEmpty() ? out : err;
    if (combined.contains(QStringLiteral("rate limit"), Qt::CaseInsensitive)
        || combined.contains(QStringLiteral("HTTP status 403"))) {
        return QStringLiteral("err_rate_limit");
    }
    if (run.crashed && !combined.isEmpty()) {
        return QStringLiteral("err_msg:") + combined;
    }
    if (!combined.isEmpty()) {
        return QStringLiteral("err_msg:") + combined;
    }
    if (run.crashed) {
        return QStringLiteral("err_updater_crash");
    }
    return QStringLiteral("err_check_failed");
}

} // namespace

UpdateService::UpdateService(QObject *parent)
    : QObject(parent)
{
}

bool UpdateService::isAppImage() const
{
    return qEnvironmentVariableIsSet("APPIMAGE");
}

bool UpdateService::updatesDisabled() const
{
    const auto disabled = [](const char *key) {
        const QByteArray v = qgetenv(key).toLower();
        return v == "1" || v == "true" || v == "yes";
    };
    return disabled("WEBAPPSTATION_NO_UPDATE")
        || disabled("APPIMAGE_UPDATE_DISABLE");
}

QString UpdateService::appImagePath() const
{
    return QString::fromLocal8Bit(qgetenv("APPIMAGE"));
}

QString UpdateService::configPath() const
{
    if (qEnvironmentVariableIsSet("WEBAPPSTATION_CONFIG_DIR")) {
        return QDir(qEnvironmentVariable("WEBAPPSTATION_CONFIG_DIR"))
            .filePath(QStringLiteral("update.conf"));
    }
    return QDir(QStandardPaths::writableLocation(QStandardPaths::ConfigLocation))
        .filePath(QStringLiteral("webappstation/update.conf"));
}

UpdateService::Pref UpdateService::loadPref() const
{
    QFile file(configPath());
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return Pref::NotAsked;
    }
    QTextStream in(&file);
    while (!in.atEnd()) {
        const QString line = in.readLine().trimmed();
        if (line.startsWith(QStringLiteral("auto_update="))) {
            const QString value = line.mid(12).trimmed().toLower();
            if (value == QStringLiteral("true") || value == QStringLiteral("1")
                || value == QStringLiteral("yes")) {
                return Pref::Enabled;
            }
            if (value == QStringLiteral("false") || value == QStringLiteral("0")
                || value == QStringLiteral("no")) {
                return Pref::Disabled;
            }
        }
    }
    return Pref::NotAsked;
}

QString UpdateService::autoUpdatePref() const
{
    switch (loadPref()) {
    case Pref::Enabled:
        return QStringLiteral("enabled");
    case Pref::Disabled:
        return QStringLiteral("disabled");
    case Pref::NotAsked:
    default:
        return QStringLiteral("notasked");
    }
}

void UpdateService::savePref(bool enabled)
{
    const QString path = configPath();
    QDir().mkpath(QFileInfo(path).absolutePath());
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return;
    }
    QTextStream out(&file);
    out << "auto_update=" << (enabled ? "true" : "false") << '\n';
    Q_EMIT prefChanged();
}

void UpdateService::setBusy(bool busy)
{
    if (m_busy == busy) {
        return;
    }
    m_busy = busy;
    Q_EMIT busyChanged();
}

void UpdateService::checkAndApply(bool manual)
{
    if (!isAppImage() || updatesDisabled() || m_busy) {
        if (manual && !isAppImage()) {
            Q_EMIT updateFailed(
                i18n("Atualizações só funcionam no AppImage."));
        }
        return;
    }

    setBusy(true);
    if (manual) {
        Q_EMIT updateMessage(i18n("Verificando atualizações..."));
    }

    const QString path = appImagePath();
    const QStringList tools = updaterCandidates();
    auto *watcher = new QFutureWatcher<QString>(this);
    connect(watcher, &QFutureWatcher<QString>::finished, this,
            [this, watcher, manual]() {
                const QString result = watcher->result();
                watcher->deleteLater();
                setBusy(false);

                if (result == QStringLiteral("uptodate")) {
                    if (manual) {
                        Q_EMIT upToDate();
                    }
                    return;
                }
                if (result.startsWith(QStringLiteral("updated:"))) {
                    Q_EMIT updateApplied(result.mid(8));
                    return;
                }
                if (result == QStringLiteral("err_start_updater")) {
                    Q_EMIT updateFailed(
                        i18n("Não foi possível iniciar o atualizador."));
                    return;
                }
                if (result == QStringLiteral("err_no_tool")) {
                    Q_EMIT updateFailed(i18n(
                        "Ferramenta de atualização (appimageupdatetool) não "
                        "encontrada. Baixe a nova release em GitHub."));
                    return;
                }
                if (result == QStringLiteral("err_rate_limit")) {
                    Q_EMIT updateFailed(i18n(
                        "GitHub limitou consultas à API (rate limit). Tente de "
                        "novo em alguns minutos."));
                    return;
                }
                if (result == QStringLiteral("err_updater_crash")) {
                    Q_EMIT updateFailed(i18n(
                        "O atualizador encerrou de forma inesperada ao "
                        "consultar o GitHub."));
                    return;
                }
                if (result == QStringLiteral("err_check_failed")) {
                    Q_EMIT updateFailed(
                        i18n("Falha ao verificar atualizações."));
                    return;
                }
                if (result.startsWith(QStringLiteral("err_msg:"))) {
                    Q_EMIT updateFailed(result.mid(8));
                    return;
                }
                Q_EMIT updateFailed(result);
            });

    watcher->setFuture(QtConcurrent::run([path, tools]() -> QString {
        QString lastError;
        bool foundTool = false;

        for (const QString &candidate : tools) {
            const QString tool = resolveUpdaterTool(candidate);
            if (tool.isEmpty()) {
                continue;
            }
            foundTool = true;

            // -j: exit 0 = sem update, 1 = update disponível, outro = erro.
            const UpdaterRunResult check =
                runUpdater(tool, {QStringLiteral("-j"), path});
            if (!check.started) {
                lastError = QStringLiteral("err_start_updater");
                continue;
            }
            if (check.crashed) {
                return updaterFailureMessage(check);
            }

            const QByteArray &out = check.standardOutput;
            const bool jsonSaysUpdate =
                out.contains("\"update_available\": true")
                || out.contains("\"update_available\":true");
            const bool hasUpdate = (check.exitCode == 1) || jsonSaysUpdate;

            if (check.exitCode == 0 && !hasUpdate) {
                return QStringLiteral("uptodate");
            }
            if (!hasUpdate) {
                return updaterFailureMessage(check);
            }

            const UpdaterRunResult apply =
                runUpdater(tool, {QStringLiteral("-O"), path});
            if (!apply.started) {
                return QStringLiteral("err_start_updater");
            }
            if (apply.crashed || apply.exitCode != 0) {
                return updaterFailureMessage(apply);
            }
            return QStringLiteral("updated:") + path;
        }

        if (!foundTool) {
            return QStringLiteral("err_no_tool");
        }
        return lastError.isEmpty() ? QStringLiteral("err_start_updater")
                                   : lastError;
    }));
}

void UpdateService::restart()
{
    const QString path = appImagePath();
    if (path.isEmpty()) {
        return;
    }
    if (!QProcess::startDetached(path, {})) {
        Q_EMIT updateFailed(i18n(
            "Não foi possível reiniciar a Estação de Aplicativos Web."));
        return;
    }
    QCoreApplication::quit();
}
