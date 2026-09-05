// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

Kirigami.ScrollablePage {
    id: page
    title: i18n("Web Apps")

    actions: [
        Kirigami.Action {
            text: i18n("Add")
            icon.name: "list-add"
            shortcut: "Ctrl+N"
            enabled: App.hasBrowsers
            tooltip: App.hasBrowsers ? i18n("Add a New Web App")
                                     : i18n("No supported browsers were detected.")
            onTriggered: pageStack.push(Qt.resolvedUrl("WebAppEditorPage.qml"), {
                editMode: false,
                editIndex: -1
            })
        },
        Kirigami.Action {
            text: i18n("Edit")
            icon.name: "document-edit"
            shortcut: "Ctrl+E"
            enabled: list.currentIndex >= 0
            onTriggered: pageStack.push(Qt.resolvedUrl("WebAppEditorPage.qml"), {
                editMode: true,
                editIndex: list.currentIndex
            })
        },
        Kirigami.Action {
            text: i18n("Remove")
            icon.name: "edit-delete"
            shortcut: "Ctrl+D"
            enabled: list.currentIndex >= 0
            onTriggered: deleteDialog.open()
        },
        Kirigami.Action {
            text: i18n("Launch")
            icon.name: "media-playback-start"
            enabled: list.currentIndex >= 0
            onTriggered: App.launchWebApp(list.currentIndex)
        },
        Kirigami.Action {
            text: i18n("Check for updates")
            icon.name: "system-software-update"
            visible: App.updateService.appImage && !App.updateService.updatesDisabled()
            enabled: !App.updateService.busy
            onTriggered: App.updateService.checkAndApply(true)
        }
    ]

    Kirigami.PlaceholderMessage {
        anchors.centerIn: parent
        width: parent.width - Kirigami.Units.gridUnit * 4
        visible: App.webApps.rowCount() === 0
        text: i18n("No Web Apps yet")
        explanation: App.hasBrowsers
            ? i18n("Run websites as if they were apps")
            : i18n("No supported browsers were detected.")
        helpfulAction: Kirigami.Action {
            text: i18n("Add")
            icon.name: "list-add"
            enabled: App.hasBrowsers
            onTriggered: pageStack.push(Qt.resolvedUrl("WebAppEditorPage.qml"), {
                editMode: false,
                editIndex: -1
            })
        }
    }

    ListView {
        id: list
        visible: App.webApps.rowCount() > 0
        model: App.webApps
        currentIndex: count > 0 ? 0 : -1
        clip: true

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

    Controls.Dialog {
        id: deleteDialog
        title: list.currentIndex >= 0
            ? i18n("Delete '%1'", App.webAppAt(list.currentIndex).name)
            : i18n("Remove")
        modal: true
        anchors.centerIn: parent
        standardButtons: Controls.Dialog.Cancel | Controls.Dialog.Yes
        contentItem: ColumnLayout {
            Controls.Label {
                text: list.currentIndex >= 0
                    ? i18n("Are you sure you want to delete '%1'?", App.webAppAt(list.currentIndex).name)
                    : ""
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }
            Controls.Label {
                text: i18n("This Web App will be permanently lost.")
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
                opacity: 0.8
            }
        }
        onAccepted: {
            App.deleteWebApp(list.currentIndex)
            list.currentIndex = App.webApps.rowCount() > 0 ? 0 : -1
        }
    }

    Keys.onEscapePressed: App.refresh()
}
