// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

Kirigami.ScrollablePage {
    id: page
    title: i18n("Escolher um ícone")
    property var onIconChosen

    property var icons: []

    Connections {
        target: App.faviconService
        function onIconsFound(found) {
            page.icons = found
        }
    }

    actions: [
        Kirigami.Action {
            text: i18n("Cancelar")
            icon.name: "dialog-cancel"
            onTriggered: pageStack.pop()
        }
    ]

    Controls.BusyIndicator {
        anchors.centerIn: parent
        running: App.faviconService.busy
        visible: running
    }

    Kirigami.PlaceholderMessage {
        anchors.centerIn: parent
        visible: !App.faviconService.busy && icons.length === 0
        text: i18n("Nenhum ícone encontrado")
    }

    Flow {
        width: parent.width
        spacing: Kirigami.Units.largeSpacing
        visible: !App.faviconService.busy

        Repeater {
            model: icons
            delegate: Controls.Button {
                contentItem: ColumnLayout {
                    Kirigami.Icon {
                        source: modelData.path
                        Layout.preferredWidth: Kirigami.Units.iconSizes.huge
                        Layout.preferredHeight: Kirigami.Units.iconSizes.huge
                        Layout.alignment: Qt.AlignHCenter
                    }
                    Controls.Label {
                        text: modelData.width + "x" + modelData.height
                        Layout.alignment: Qt.AlignHCenter
                    }
                }
                onClicked: {
                    App.faviconService.selectIcon(modelData.path)
                    pageStack.pop()
                }
            }
        }
    }
}
