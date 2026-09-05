// SPDX-License-Identifier: MIT
#pragma once

#include <QObject>
#include <QString>

class UpdateService : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool appImage READ isAppImage CONSTANT)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(QString autoUpdatePref READ autoUpdatePref NOTIFY prefChanged)

public:
    enum class Pref { NotAsked, Enabled, Disabled };
    Q_ENUM(Pref)

    explicit UpdateService(QObject *parent = nullptr);

    [[nodiscard]] bool isAppImage() const;
    [[nodiscard]] bool busy() const { return m_busy; }
    [[nodiscard]] QString autoUpdatePref() const;
    [[nodiscard]] Pref loadPref() const;

    Q_INVOKABLE void savePref(bool enabled);
    Q_INVOKABLE void checkAndApply(bool manual);
    Q_INVOKABLE void restart();
    Q_INVOKABLE bool updatesDisabled() const;

Q_SIGNALS:
    void busyChanged();
    void prefChanged();
    void updateMessage(const QString &message);
    void updateApplied(const QString &appImagePath);
    void updateFailed(const QString &error);
    void upToDate();

private:
    void setBusy(bool busy);
    [[nodiscard]] QString configPath() const;
    [[nodiscard]] QString appImagePath() const;

    bool m_busy = false;
};
