// SPDX-License-Identifier: MIT
#include <QFile>
#include <QTest>

class TstQmlSources : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void list_page_is_portuguese()
    {
        QFile f(QStringLiteral(SOURCE_DIR "/src/qml/pages/WebAppListPage.qml"));
        QVERIFY(f.open(QIODevice::ReadOnly | QIODevice::Text));
        const QString content = QString::fromUtf8(f.readAll());
        QVERIFY(content.contains(QStringLiteral("i18n(\"Adicionar\")")));
        QVERIFY(content.contains(QStringLiteral("i18n(\"Editar\")")));
        QVERIFY(content.contains(QStringLiteral("i18n(\"Remover\")")));
        QVERIFY(content.contains(QStringLiteral("i18n(\"Abrir\")")));
    }

    void about_and_shortcuts_are_portuguese()
    {
        QFile about(QStringLiteral(SOURCE_DIR "/src/qml/pages/AboutPage.qml"));
        QVERIFY(about.open(QIODevice::ReadOnly | QIODevice::Text));
        const QString aboutText = QString::fromUtf8(about.readAll());
        QVERIFY(aboutText.contains(QStringLiteral("i18n(\"Sobre\")")));
        QVERIFY(aboutText.contains(
            QStringLiteral("i18n(\"Execute sites como se fossem aplicativos\")")));

        QFile shortcuts(
            QStringLiteral(SOURCE_DIR "/src/qml/pages/ShortcutsPage.qml"));
        QVERIFY(shortcuts.open(QIODevice::ReadOnly | QIODevice::Text));
        const QString shortcutsText = QString::fromUtf8(shortcuts.readAll());
        QVERIFY(shortcutsText.contains(QStringLiteral("i18n(\"Atalhos de teclado\")")));
    }

    void editor_uses_param_utils()
    {
        QFile f(QStringLiteral(SOURCE_DIR "/src/qml/pages/WebAppEditorPage.qml"));
        QVERIFY(f.open(QIODevice::ReadOnly | QIODevice::Text));
        const QString content = QString::fromUtf8(f.readAll());
        QVERIFY(content.contains(QStringLiteral("ParamUtils")));
        QVERIFY(content.contains(QStringLiteral("--start-maximized")));
    }
};

QTEST_GUILESS_MAIN(TstQmlSources)
#include "tst_qmlsources.moc"
