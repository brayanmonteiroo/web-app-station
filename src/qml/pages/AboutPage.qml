// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

Kirigami.ScrollablePage {
    title: i18n("Sobre")

    actions: [
        Kirigami.Action {
            text: i18n("Voltar")
            icon.name: "go-previous"
            onTriggered: pageStack.pop()
        }
    ]

    ColumnLayout {
        anchors.centerIn: parent
        width: Math.min(parent.width - Kirigami.Units.gridUnit * 2, Kirigami.Units.gridUnit * 28)
        spacing: Kirigami.Units.largeSpacing

        Kirigami.Icon {
            source: "org.kde.webappstation"
            Layout.preferredWidth: Kirigami.Units.iconSizes.huge
            Layout.preferredHeight: Kirigami.Units.iconSizes.huge
            Layout.alignment: Qt.AlignHCenter
        }

        Kirigami.Heading {
            text: i18n("Web Apps")
            Layout.alignment: Qt.AlignHCenter
            level: 1
        }

        Controls.Label {
            text: i18n("Execute sites como se fossem aplicativos")
            wrapMode: Text.WordWrap
            horizontalAlignment: Text.AlignHCenter
            Layout.fillWidth: true
        }

        Controls.Label {
            text: App.version
            horizontalAlignment: Text.AlignHCenter
            Layout.fillWidth: true
            opacity: 0.7
        }

        Controls.Label {
            text: "© 2026 Brayan Monteiro\nMIT License"
            wrapMode: Text.WordWrap
            horizontalAlignment: Text.AlignHCenter
            Layout.fillWidth: true
        }

        Controls.Label {
            text: "https://github.com/brayanmonteiroo/web-app-station"
            wrapMode: Text.WrapAnywhere
            horizontalAlignment: Text.AlignHCenter
            Layout.fillWidth: true
            color: Kirigami.Theme.linkColor
        }
    }
}
