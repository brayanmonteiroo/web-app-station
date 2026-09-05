// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

Kirigami.ApplicationWindow {
    id: root
    title: i18n("Web Apps")
    minimumWidth: Kirigami.Units.gridUnit * 28
    minimumHeight: Kirigami.Units.gridUnit * 24
    width: Kirigami.Units.gridUnit * 46
    height: Kirigami.Units.gridUnit * 34

    // Evita coluna estreita ao lado da lista (formulário cortado)
    pageStack.columnView.columnResizeMode: Kirigami.ColumnView.SingleColumn

    pageStack.initialPage: WebAppListPage {}

    globalDrawer: Kirigami.GlobalDrawer {
        title: i18n("Web App Station")
        titleIcon: "org.kde.webappstation"
        isMenu: true
        actions: [
            Kirigami.Action {
                text: i18n("Atalhos de teclado")
                icon.name: "configure-shortcuts"
                shortcut: "Ctrl+K"
                onTriggered: root.pageStack.push(Qt.createComponent("org.kde.webappstation", "ShortcutsPage"))
            },
            Kirigami.Action {
                text: i18n("Sobre")
                icon.name: "help-about"
                shortcut: "F1"
                onTriggered: root.pageStack.push(Qt.createComponent("org.kde.webappstation", "AboutPage"))
            },
            Kirigami.Action {
                text: i18n("Sair")
                icon.name: "application-exit"
                shortcut: "Ctrl+Q"
                onTriggered: Qt.quit()
            }
        ]
    }

    Connections {
        target: App.updateService
        function onUpdateMessage(message) {
            applicationWindow().showPassiveNotification(message)
        }
        function onUpToDate() {
            applicationWindow().showPassiveNotification(i18n("Já está na versão mais recente."))
        }
        function onUpdateFailed(error) {
            applicationWindow().showPassiveNotification(error)
        }
        function onUpdateApplied(path) {
            restartDialog.appImagePath = path
            restartDialog.open()
        }
    }

    Component.onCompleted: {
        if (!App.updateService.appImage || App.updateService.updatesDisabled()) {
            return
        }
        const pref = App.updateService.autoUpdatePref
        if (pref === "notasked") {
            consentDialog.open()
        } else if (pref === "enabled") {
            autoUpdateTimer.start()
        }
    }

    Timer {
        id: autoUpdateTimer
        interval: 2000
        repeat: false
        onTriggered: App.updateService.checkAndApply(false)
    }

    Controls.Dialog {
        id: consentDialog
        title: i18n("Verificar atualizações?")
        modal: true
        anchors.centerIn: parent
        standardButtons: Controls.Dialog.Yes | Controls.Dialog.No
        contentItem: Controls.Label {
            text: i18n("Deseja que o Web App Station verifique atualizações automaticamente quando aberto pelo AppImage?")
            wrapMode: Text.WordWrap
            width: Kirigami.Units.gridUnit * 24
        }
        onAccepted: {
            App.updateService.savePref(true)
            App.updateService.checkAndApply(false)
        }
        onRejected: App.updateService.savePref(false)
    }

    Controls.Dialog {
        id: restartDialog
        property string appImagePath: ""
        title: i18n("Atualização instalada")
        modal: true
        anchors.centerIn: parent
        contentItem: Controls.Label {
            text: i18n("A atualização foi aplicada. Reiniciar agora?")
            wrapMode: Text.WordWrap
            width: Kirigami.Units.gridUnit * 24
        }
        footer: Controls.DialogButtonBox {
            Controls.Button {
                text: i18n("Reiniciar agora")
                Controls.DialogButtonBox.buttonRole: Controls.DialogButtonBox.AcceptRole
                onClicked: {
                    restartDialog.close()
                    App.updateService.restart()
                }
            }
            Controls.Button {
                text: i18n("Depois")
                Controls.DialogButtonBox.buttonRole: Controls.DialogButtonBox.RejectRole
                onClicked: restartDialog.close()
            }
        }
    }
}
