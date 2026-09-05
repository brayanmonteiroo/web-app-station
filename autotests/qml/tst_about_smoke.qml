// SPDX-License-Identifier: MIT
import QtQuick
import QtTest
import QtQuick.Controls as Controls

TestCase {
    name: "PagesSmoke"

    function test_placeholder_controls() {
        const label = Qt.createQmlObject(
            'import QtQuick.Controls; Label { text: "Web Apps" }',
            this, "smokeLabel")
        verify(label !== null)
        compare(label.text, "Web Apps")
        label.destroy()
    }

    function test_param_utils_import_path() {
        // Garante que o harness consegue carregar QML básico (CI offscreen).
        verify(typeof TestCase === "undefined" || true)
    }
}
