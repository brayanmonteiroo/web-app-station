// SPDX-License-Identifier: MIT
#include <KLocalizedString>

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QTest>

#include "LocaleService.h"

#include "common/TempHome.h"

class TstLocale : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase()
    {
        QVERIFY(m_home.isValid());
        const QString localeRoot = QDir::cleanPath(
            QCoreApplication::applicationDirPath()
            + QStringLiteral("/../locale"));
        QVERIFY2(QFile::exists(localeRoot
                               + QStringLiteral(
                                   "/en_GB/LC_MESSAGES/webappstation.mo")),
                 qPrintable(QStringLiteral("Missing en_GB catalog at %1")
                                .arg(localeRoot)));
        qputenv("WEBAPPSTATION_LOCALE_DIR", QFile::encodeName(localeRoot));

        KLocalizedString::setApplicationDomain("webappstation");
        LocaleService::finishI18nSetup();
    }

    void english_translates_core_ui_strings()
    {
        LocaleService locale;
        locale.setLanguage(QStringLiteral("en"));
        LocaleService::finishI18nSetup();
        QCOMPARE(i18n("Adicionar"), QStringLiteral("Add"));
        QCOMPARE(i18n("Editar"), QStringLiteral("Edit"));
        QCOMPARE(i18n("Remover"), QStringLiteral("Remove"));
        QCOMPARE(i18n("Abrir"), QStringLiteral("Launch"));
        QCOMPARE(i18n("Aplicativos Web"), QStringLiteral("Web Apps"));
        QCOMPARE(i18n("Estação de Aplicativos Web"),
                 QStringLiteral("Web App Station"));
        QCOMPARE(i18n("Adicionar aplicativo web"),
                 QStringLiteral("Add a new web app"));
        QCOMPARE(i18n("Idioma"), QStringLiteral("Language"));
        QCOMPARE(i18n("Atualizações"), QStringLiteral("Updates"));
        QCOMPARE(i18n("Sair"), QStringLiteral("Quit"));
    }

    void portuguese_keeps_source_msgids()
    {
        LocaleService locale;
        locale.setLanguage(QStringLiteral("pt_BR"));
        LocaleService::finishI18nSetup();
        QCOMPARE(i18n("Adicionar"), QStringLiteral("Adicionar"));
        QCOMPARE(i18n("Aplicativos Web"), QStringLiteral("Aplicativos Web"));
        QCOMPARE(i18n("Estação de Aplicativos Web"),
                 QStringLiteral("Estação de Aplicativos Web"));
        QCOMPARE(i18n("Adicionar aplicativo web"),
                 QStringLiteral("Adicionar aplicativo web"));
        QCOMPARE(i18n("Idioma"), QStringLiteral("Idioma"));
    }

    void locale_service_persists_and_applies_override()
    {
        LocaleService locale;
        locale.setLanguage(QStringLiteral("en"));
        QCOMPARE(locale.language(), QStringLiteral("en"));
        LocaleService::finishI18nSetup();
        QCOMPARE(i18n("Adicionar"), QStringLiteral("Add"));

        locale.setLanguage(QStringLiteral("pt_BR"));
        QCOMPARE(locale.language(), QStringLiteral("pt_BR"));
        LocaleService::finishI18nSetup();
        QCOMPARE(i18n("Adicionar"), QStringLiteral("Adicionar"));
    }

private:
    TempHome m_home;
};

QTEST_GUILESS_MAIN(TstLocale)
#include "tst_locale.moc"
