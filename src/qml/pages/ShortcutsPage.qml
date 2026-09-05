// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

Kirigami.ScrollablePage {
    title: i18n("Atalhos de teclado")

    actions: [
        Kirigami.Action {
            text: i18n("Voltar")
            icon.name: "go-previous"
            onTriggered: pageStack.pop()
        }
    ]

    Kirigami.FormLayout {
        wideMode: true

        Controls.Label {
            Kirigami.FormData.label: "Ctrl+N"
            text: i18n("Adicionar")
        }
        Controls.Label {
            Kirigami.FormData.label: "Ctrl+E"
            text: i18n("Editar")
        }
        Controls.Label {
            Kirigami.FormData.label: "Ctrl+D"
            text: i18n("Remover")
        }
        Controls.Label {
            Kirigami.FormData.label: "Esc"
            text: i18n("Voltar")
        }
        Controls.Label {
            Kirigami.FormData.label: "Ctrl+K"
            text: i18n("Atalhos de teclado")
        }
        Controls.Label {
            Kirigami.FormData.label: "F1"
            text: i18n("Sobre")
        }
        Controls.Label {
            Kirigami.FormData.label: "Ctrl+Q / Ctrl+W"
            text: i18n("Sair")
        }
    }
}
