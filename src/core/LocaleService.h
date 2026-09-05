// SPDX-License-Identifier: MIT
#pragma once

#include <QObject>
#include <QString>

class LocaleService : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString language READ language NOTIFY languageChanged)

public:
    explicit LocaleService(QObject *parent = nullptr);

    /** Preferência via LANG/LANGUAGE antes do QApplication. */
    static void applyFromConfigEarly();

    /**
     * Registra catálogos (.mo) e aplica override de idioma no KI18n.
     * Chamar depois de setApplicationDomain().
     */
    static void finishI18nSetup();

    [[nodiscard]] QString language() const;
    Q_INVOKABLE void setLanguage(const QString &code);
    Q_INVOKABLE void restartApp();

Q_SIGNALS:
    void languageChanged();
    void restartRequired();
    void restartFailed(const QString &error);

private:
    [[nodiscard]] static QString configPath();
    [[nodiscard]] static QString loadLanguageCode();
    static void saveLanguageCode(const QString &code);
    static void applyLanguageEnv(const QString &code);
    static void registerLocaleDirs();
    static void applyLanguageOverride(const QString &code);
};
