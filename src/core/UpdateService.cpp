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
    const QString besideApp =
        QCoreApplication::applicationDirPath()
        + QStringLiteral("/appimageupdatetool");
    if (!tools.contains(besideApp)) {
        tools << besideApp;
    }
    tools << QStringLiteral("appimageupdatetool")
          << QStringLiteral("AppImageUpdate");
    return tools;
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

bool runUpdater(const QString &tool, const QStringList &args, QByteArray *stdoutOut,
                QByteArray *stderrOut, int *exitCode)
{
    QProcess proc;
    prepareUpdaterEnvironment(proc, tool);
    proc.start(tool, args);
    if (!proc.waitForStarted(5000)) {
        return false;
    }
    proc.waitForFinished(-1);
    if (stdoutOut) {
        *stdoutOut = proc.readAllStandardOutput();
    }
    if (stderrOut) {
        *stderrOut = proc.readAllStandardError();
    }
    if (exitCode) {
        *exitCode = proc.exitCode();
    }
    return true;
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
                Q_EMIT updateFailed(result);
            });

    watcher->setFuture(QtConcurrent::run([path, tools]() -> QString {
        for (const QString &tool : tools) {
            if (QFileInfo::exists(tool) && !QFileInfo(tool).isExecutable()
                && QFileInfo(tool).isAbsolute()) {
                continue;
            }

            QByteArray out;
            QByteArray err;
            int code = -1;
            if (!runUpdater(tool, {QStringLiteral("-j"), path}, &out, &err,
                            &code)) {
                continue;
            }

            const bool hasUpdate = out.contains("\"update_available\": true")
                || out.contains("\"update_available\":true");
            if (code != 0 && !hasUpdate && !out.contains("update_available")) {
                continue;
            }
            if (!hasUpdate) {
                return QStringLiteral("uptodate");
            }

            QByteArray applyOut;
            QByteArray applyErr;
            int applyCode = -1;
            if (!runUpdater(tool, {QStringLiteral("-O"), path}, &applyOut,
                            &applyErr, &applyCode)) {
                return QStringLiteral("err_start_updater");
            }
            if (applyCode != 0) {
                const QString msg =
                    QString::fromUtf8(applyErr).trimmed();
                return msg.isEmpty() ? QString::fromUtf8(applyOut).trimmed()
                                     : msg;
            }
            return QStringLiteral("updated:") + path;
        }

        return QStringLiteral("err_no_tool");
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
