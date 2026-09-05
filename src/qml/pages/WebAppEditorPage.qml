// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import "ParamUtils.js" as ParamUtils

Kirigami.ScrollablePage {
    id: page
    property bool editMode: false
    property int editIndex: -1

    title: editMode ? i18n("Editar aplicativo web") : i18n("Adicionar aplicativo web")

    property string selectedIcon: "org.kde.webappstation"
    property int browserIndex: 0
    property int categoryIndex: 0

    Component.onCompleted: {
        if (editMode && editIndex >= 0) {
            const app = App.webAppAt(editIndex)
            nameField.text = app.name || ""
            descField.text = app.description || ""
            urlField.text = app.url || ""
            selectedIcon = app.icon || "org.kde.webappstation"
            customField.text = app.customParameters || ""
            isolateSwitch.checked = !!app.isolateProfile
            navbarSwitch.checked = !!app.navbar
            privateSwitch.checked = !!app.privateWindow
            browserIndex = App.browserIndexForName(app.browser || "")
            for (let i = 0; i < App.categories.length; ++i) {
                if (App.categories[i].id === app.category) {
                    categoryIndex = i
                    break
                }
            }
        } else {
            isolateSwitch.checked = true
        }
        syncBrowserWidgets()
    }

    function syncBrowserWidgets() {
        if (App.browsers.length === 0) {
            return
        }
        const b = App.browsers[Math.min(browserIndex, App.browsers.length - 1)]
        isolateSection.visible = !!b.supportsIsolation
        navbarSection.visible = !!b.supportsNavbar
    }

    function save() {
        const payload = {
            name: nameField.text.trim(),
            description: descField.text.trim(),
            url: App.normalizeUrl(urlField.text),
            icon: selectedIcon,
            category: App.categories[categoryIndex].id,
            browserIndex: browserIndex,
            customParameters: customField.text.trim(),
            isolateProfile: isolateSwitch.checked,
            navbar: navbarSwitch.checked,
            privateWindow: privateSwitch.checked
        }
        const ok = editMode ? App.editWebApp(editIndex, payload)
                            : App.createWebApp(payload)
        if (ok) {
            pageStack.pop()
        }
    }

    actions: [
        Kirigami.Action {
            text: i18n("Salvar")
            icon.name: "dialog-ok"
            enabled: nameField.text.trim().length > 0 && urlField.text.trim().length > 0
            onTriggered: page.save()
        },
        Kirigami.Action {
            text: i18n("Cancelar")
            icon.name: "dialog-cancel"
            onTriggered: pageStack.pop()
        }
    ]

    ColumnLayout {
        width: Math.min(page.width - page.leftPadding - page.rightPadding,
                        Kirigami.Units.gridUnit * 36)
        spacing: Kirigami.Units.largeSpacing

        Controls.Label {
            text: i18n("Nome")
            font.bold: true
        }
        Controls.TextField {
            id: nameField
            Layout.fillWidth: true
            placeholderText: i18n("Nome do site")
        }

        Controls.Label {
            text: i18n("Descrição")
            font.bold: true
        }
        Controls.TextField {
            id: descField
            Layout.fillWidth: true
            placeholderText: i18n("Aplicativo web")
        }

        Controls.Label {
            text: i18n("Endereço")
            font.bold: true
        }
        Controls.TextField {
            id: urlField
            Layout.fillWidth: true
            placeholderText: "https://www.exemplo.com"
            onTextChanged: {
                const guess = App.faviconService.guessThemeIcon(App.normalizeUrl(text))
                if (guess.length > 0 && !editMode) {
                    selectedIcon = guess
                }
            }
        }

        Controls.Label {
            text: i18n("Ícone")
            font.bold: true
        }
        RowLayout {
            Layout.fillWidth: true
            spacing: Kirigami.Units.largeSpacing
            Kirigami.Icon {
                source: selectedIcon
                Layout.preferredWidth: Kirigami.Units.iconSizes.large
                Layout.preferredHeight: Kirigami.Units.iconSizes.large
            }
            Controls.Button {
                text: i18n("Buscar ícones online")
                Layout.fillWidth: true
                enabled: urlField.text.trim().length > 0 && !App.faviconService.busy
                onClicked: {
                    App.faviconService.findIcons(App.normalizeUrl(urlField.text))
                    pageStack.push(Qt.createComponent("org.kde.webappstation", "FaviconPage"))
                }
            }
        }

        Connections {
            target: App.faviconService
            function onIconSelected(path) {
                selectedIcon = path
            }
        }

        Controls.Label {
            text: i18n("Categoria")
            font.bold: true
        }
        Controls.ComboBox {
            id: categoryCombo
            Layout.fillWidth: true
            model: App.categories
            textRole: "name"
            currentIndex: categoryIndex
            onCurrentIndexChanged: categoryIndex = currentIndex
        }

        Controls.Label {
            text: i18n("Navegador")
            font.bold: true
            visible: !editMode && App.browsers.length > 1
        }
        Controls.ComboBox {
            id: browserCombo
            Layout.fillWidth: true
            visible: !editMode && App.browsers.length > 1
            model: App.browsers
            textRole: "name"
            currentIndex: browserIndex
            onCurrentIndexChanged: {
                browserIndex = currentIndex
                page.syncBrowserWidgets()
            }
        }

        ColumnLayout {
            id: isolateSection
            Layout.fillWidth: true
            spacing: Kirigami.Units.smallSpacing
            RowLayout {
                Layout.fillWidth: true
                Controls.Label {
                    text: i18n("Perfil isolado")
                    Layout.fillWidth: true
                }
                Controls.Switch {
                    id: isolateSwitch
                }
            }
            Controls.Label {
                Layout.fillWidth: true
                text: i18n("Se ativado, o site roda com um perfil próprio (login separado do browser). Se desativado no Chrome/Brave, usa a mesma sessão — você já entra logado quando estiver logado no navegador.")
                wrapMode: Text.WordWrap
                opacity: 0.75
                font: Kirigami.Theme.smallFont
            }
        }

        ColumnLayout {
            id: navbarSection
            visible: false
            Layout.fillWidth: true
            spacing: Kirigami.Units.smallSpacing
            RowLayout {
                Layout.fillWidth: true
                Controls.Label {
                    text: i18n("Barra de navegação")
                    Layout.fillWidth: true
                }
                Controls.Switch {
                    id: navbarSwitch
                }
            }
            Controls.Label {
                Layout.fillWidth: true
                text: i18n("Mostra a barra de navegação do Firefox neste aplicativo web.")
                wrapMode: Text.WordWrap
                opacity: 0.75
                font: Kirigami.Theme.smallFont
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: Kirigami.Units.smallSpacing
            RowLayout {
                Layout.fillWidth: true
                Controls.Label {
                    text: i18n("Janela privada / anônima")
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                }
                Controls.Switch {
                    id: privateSwitch
                }
            }
        }

        Controls.Label {
            text: i18n("Parâmetros extras")
            font.bold: true
        }
        Controls.TextField {
            id: customField
            Layout.fillWidth: true
            placeholderText: i18n("Ex.: --start-maximized")
        }
        Controls.Label {
            Layout.fillWidth: true
            text: i18n("Atalhos comuns (Chromium / Brave / Chrome):")
            wrapMode: Text.WordWrap
            opacity: 0.75
            font: Kirigami.Theme.smallFont
        }
        Flow {
            Layout.fillWidth: true
            spacing: Kirigami.Units.smallSpacing

            Repeater {
                model: [
                    { label: i18n("Maximizado"), flag: "--start-maximized" },
                    { label: i18n("Tela cheia"), flag: "--start-fullscreen" },
                    { label: i18n("Nova janela"), flag: "--new-window" },
                    { label: i18n("Sem extensões"), flag: "--disable-extensions" }
                ]
                delegate: Controls.Button {
                    text: modelData.label
                    checkable: true
                    checked: ParamUtils.hasFlag(customField.text, modelData.flag)
                    onClicked: customField.text = ParamUtils.toggle(customField.text, modelData.flag)
                }
            }
        }

        // Espaço extra para o scroll não colar no fim
        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: Kirigami.Units.gridUnit
        }
    }
}
