// SPDX-License-Identifier: MIT
#include "UpdateService.h"

#include <KLocalizedString>

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFutureWatcher>
#include <QProcess>
#include <QStandardPaths>
#include <QTextStream>
#include <QtConcurrent>

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

    watcher->setFuture(QtConcurrent::run([path]() -> QString {
        // Prefer AppImageUpdate / appimageupdatetool when available.
        const QStringList tools = {
            QCoreApplication::applicationDirPath()
                + QStringLiteral("/appimageupdatetool"),
            QStringLiteral("appimageupdatetool"),
            QStringLiteral("AppImageUpdate"),
        };

        for (const QString &tool : tools) {
            QProcess proc;
            proc.start(tool, {QStringLiteral("-j"), path});
            if (!proc.waitForStarted(3000)) {
                continue;
            }
            proc.waitForFinished(-1);
            const QByteArray out = proc.readAllStandardOutput();
            const bool hasUpdate =
                out.contains("\"update_available\": true")
                || out.contains("\"update_available\":true");
            if (proc.exitCode() != 0 && !hasUpdate
                && !out.contains("update_available")) {
                // Tool exists but check failed — try next or apply anyway path
                if (!hasUpdate) {
                    continue;
                }
            }
            if (!hasUpdate) {
                return QStringLiteral("uptodate");
            }

            QProcess apply;
            apply.start(tool, {QStringLiteral("-O"), path});
            if (!apply.waitForStarted(3000)) {
                return QStringLiteral("err_start_updater");
            }
            apply.waitForFinished(-1);
            if (apply.exitCode() != 0) {
                return QString::fromUtf8(apply.readAllStandardError()).trimmed();
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
        Q_EMIT updateFailed(
            i18n("Não foi possível reiniciar o Web App Station."));
        return;
    }
    QCoreApplication::quit();
}
