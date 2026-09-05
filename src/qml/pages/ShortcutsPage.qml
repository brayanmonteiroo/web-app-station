// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

Kirigami.ScrollablePage {
    title: i18n("Shortcuts")

    actions: [
        Kirigami.Action {
            text: i18n("Back")
            icon.name: "go-previous"
            onTriggered: pageStack.pop()
        }
    ]

    Kirigami.FormLayout {
        wideMode: true

        Controls.Label {
            Kirigami.FormData.label: "Ctrl+N"
            text: i18n("Add")
        }
        Controls.Label {
            Kirigami.FormData.label: "Ctrl+E"
            text: i18n("Edit")
        }
        Controls.Label {
            Kirigami.FormData.label: "Ctrl+D"
            text: i18n("Remove")
        }
        Controls.Label {
            Kirigami.FormData.label: "Esc"
            text: i18n("Go Back")
        }
        Controls.Label {
            Kirigami.FormData.label: "Ctrl+K"
            text: i18n("Shortcuts")
        }
        Controls.Label {
            Kirigami.FormData.label: "F1"
            text: i18n("About")
        }
        Controls.Label {
            Kirigami.FormData.label: "Ctrl+Q / Ctrl+W"
            text: i18n("Quit")
        }
    }
}
