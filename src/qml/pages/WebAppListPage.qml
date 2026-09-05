// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

Kirigami.Page {
    id: page
    title: i18n("Web Apps")

    actions: [
        Kirigami.Action {
            text: i18n("Adicionar")
            icon.name: "list-add"
            shortcut: "Ctrl+N"
            enabled: App.hasBrowsers
            tooltip: App.hasBrowsers ? i18n("Adicionar um novo Web App")
                                     : i18n("Nenhum navegador suportado foi detectado.")
            onTriggered: pageStack.push(Qt.createComponent("org.kde.webappstation", "WebAppEditorPage"), {
                editMode: false,
                editIndex: -1
            })
        },
        Kirigami.Action {
            text: i18n("Editar")
            icon.name: "document-edit"
            shortcut: "Ctrl+E"
            enabled: list.currentIndex >= 0
            onTriggered: pageStack.push(Qt.createComponent("org.kde.webappstation", "WebAppEditorPage"), {
                editMode: true,
                editIndex: list.currentIndex
            })
        },
        Kirigami.Action {
            text: i18n("Remover")
            icon.name: "edit-delete"
            shortcut: "Ctrl+D"
            enabled: list.currentIndex >= 0
            onTriggered: deleteDialog.open()
        },
        Kirigami.Action {
            text: i18n("Abrir")
            icon.name: "media-playback-start"
            enabled: list.currentIndex >= 0
            onTriggered: App.launchWebApp(list.currentIndex)
        },
        Kirigami.Action {
            text: i18n("Verificar atualizações")
            icon.name: "system-software-update"
            visible: App.updateService.appImage && !App.updateService.updatesDisabled()
            enabled: !App.updateService.busy
            onTriggered: App.updateService.checkAndApply(true)
        }
    ]

    ListView {
        id: list
        anchors.fill: parent
        model: App.webApps
        currentIndex: count > 0 ? 0 : -1
        clip: true
        visible: count > 0

        delegate: Controls.ItemDelegate {
            width: ListView.view.width
            highlighted: ListView.isCurrentItem
            onClicked: list.currentIndex = index
            onDoubleClicked: App.launchWebApp(index)

            contentItem: RowLayout {
                spacing: Kirigami.Units.largeSpacing

                Kirigami.Icon {
                    source: model.icon
                    Layout.preferredWidth: Kirigami.Units.iconSizes.medium
                    Layout.preferredHeight: Kirigami.Units.iconSizes.medium
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 0
                    Controls.Label {
                        text: model.name
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }
                    Controls.Label {
                        text: model.browser
                        opacity: 0.7
                        font: Kirigami.Theme.smallFont
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }
                }
            }
        }
    }

    Kirigami.PlaceholderMessage {
        anchors.centerIn: parent
        width: parent.width - Kirigami.Units.gridUnit * 4
        visible: list.count === 0
        text: i18n("Nenhum Web App ainda")
        explanation: App.hasBrowsers
            ? i18n("Execute sites como se fossem aplicativos")
            : i18n("Nenhum navegador suportado foi detectado.")
        helpfulAction: Kirigami.Action {
            text: i18n("Adicionar")
            icon.name: "list-add"
            enabled: App.hasBrowsers
            onTriggered: pageStack.push(Qt.createComponent("org.kde.webappstation", "WebAppEditorPage"), {
                editMode: false,
                editIndex: -1
            })
        }
    }

    Controls.Dialog {
        id: deleteDialog
        title: list.currentIndex >= 0
            ? i18n("Excluir '%1'", App.webAppAt(list.currentIndex).name)
            : i18n("Remover")
        modal: true
        anchors.centerIn: parent
        standardButtons: Controls.Dialog.Cancel | Controls.Dialog.Yes
        contentItem: ColumnLayout {
            width: Kirigami.Units.gridUnit * 24
            Controls.Label {
                text: list.currentIndex >= 0
                    ? i18n("Tem certeza de que deseja excluir '%1'?", App.webAppAt(list.currentIndex).name)
                    : ""
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }
            Controls.Label {
                text: i18n("Este Web App será perdido permanentemente.")
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
                opacity: 0.8
            }
        }
        onAccepted: {
            App.deleteWebApp(list.currentIndex)
            list.currentIndex = list.count > 0 ? 0 : -1
        }
    }

    Keys.onEscapePressed: App.refresh()
}
