// SPDX-License-Identifier: MIT
#pragma once

#include <QObject>
#include <QVariantList>

class FaviconService : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)

public:
    explicit FaviconService(QObject *parent = nullptr);

    [[nodiscard]] bool busy() const { return m_busy; }

    Q_INVOKABLE void findIcons(const QString &url);
    Q_INVOKABLE QString guessThemeIcon(const QString &url) const;
    Q_INVOKABLE QString persistIcon(const QString &tempPath,
                                    const QString &appName) const;
    Q_INVOKABLE void selectIcon(const QString &path);

    [[nodiscard]] QString selectedPath() const { return m_selectedPath; }

Q_SIGNALS:
    void busyChanged();
    void iconsFound(const QVariantList &icons);
    void errorOccurred(const QString &message);
    void iconSelected(const QString &path);

private:
    void setBusy(bool busy);

    bool m_busy = false;
    QString m_selectedPath;
};
