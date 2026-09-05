// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

Kirigami.ScrollablePage {
    id: page
    property bool editMode: false
    property int editIndex: -1

    title: editMode ? i18n("Edit Web App") : i18n("Add a New Web App")

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
        isolateRow.visible = !!b.supportsIsolation
        navbarRow.visible = !!b.supportsNavbar
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
            text: i18n("OK")
            icon.name: "dialog-ok"
            enabled: nameField.text.trim().length > 0 && urlField.text.trim().length > 0
            onTriggered: page.save()
        },
        Kirigami.Action {
            text: i18n("Cancel")
            icon.name: "dialog-cancel"
            onTriggered: pageStack.pop()
        }
    ]

    Kirigami.FormLayout {
        wideMode: true

        Controls.TextField {
            id: nameField
            Kirigami.FormData.label: i18n("Name:")
            placeholderText: i18n("Website name")
        }

        Controls.TextField {
            id: descField
            Kirigami.FormData.label: i18n("Description:")
            placeholderText: i18n("Web App")
        }

        Controls.TextField {
            id: urlField
            Kirigami.FormData.label: i18n("Address:")
            placeholderText: "https://www.website.com"
            onTextChanged: {
                const guess = App.faviconService.guessThemeIcon(App.normalizeUrl(text))
                if (guess.length > 0 && !editMode) {
                    selectedIcon = guess
                }
            }
        }

        RowLayout {
            Kirigami.FormData.label: i18n("Icon:")
            Kirigami.Icon {
                source: selectedIcon
                Layout.preferredWidth: Kirigami.Units.iconSizes.large
                Layout.preferredHeight: Kirigami.Units.iconSizes.large
            }
            Controls.Button {
                text: i18n("Find icons online")
                enabled: urlField.text.trim().length > 0 && !App.faviconService.busy
                onClicked: {
                    App.faviconService.findIcons(App.normalizeUrl(urlField.text))
                    pageStack.push(Qt.resolvedUrl("FaviconPage.qml"))
                }
            }
        }

        Connections {
            target: App.faviconService
            function onIconSelected(path) {
                selectedIcon = path
            }
        }

        Controls.ComboBox {
            id: categoryCombo
            Kirigami.FormData.label: i18n("Category:")
            model: App.categories
            textRole: "name"
            currentIndex: categoryIndex
            onCurrentIndexChanged: categoryIndex = currentIndex
        }

        Controls.ComboBox {
            id: browserCombo
            Kirigami.FormData.label: i18n("Browser:")
            visible: !editMode && App.browsers.length > 1
            model: App.browsers
            textRole: "name"
            currentIndex: browserIndex
            onCurrentIndexChanged: {
                browserIndex = currentIndex
                page.syncBrowserWidgets()
            }
        }

        RowLayout {
            id: isolateRow
            Kirigami.FormData.label: i18n("Isolated profile:")
            Controls.Switch {
                id: isolateSwitch
            }
            Controls.Label {
                text: i18n("If this option is enabled the website will run with its own browser profile.")
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
                opacity: 0.8
            }
        }

        RowLayout {
            id: navbarRow
            visible: false
            Kirigami.FormData.label: i18n("Navigation bar:")
            Controls.Switch {
                id: navbarSwitch
            }
        }

        RowLayout {
            Kirigami.FormData.label: i18n("Private/Incognito Window:")
            Controls.Switch {
                id: privateSwitch
            }
        }

        Controls.TextField {
            id: customField
            Kirigami.FormData.label: i18n("Custom parameters:")
            placeholderText: i18n("Custom browser parameters")
        }
    }
}
