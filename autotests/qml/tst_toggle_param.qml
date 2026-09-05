// SPDX-License-Identifier: MIT
import QtQuick
import QtTest
import "../../src/qml/ParamUtils.js" as ParamUtils

TestCase {
    name: "ParamUtils"

    function test_toggle_adds_flag() {
        compare(ParamUtils.toggle("", "--start-maximized"), "--start-maximized")
        compare(ParamUtils.toggle("--new-window", "--start-maximized"),
                "--new-window --start-maximized")
    }

    function test_toggle_removes_flag() {
        compare(ParamUtils.toggle("--start-maximized", "--start-maximized"), "")
        compare(ParamUtils.toggle("--start-maximized --new-window", "--start-maximized"),
                "--new-window")
    }

    function test_hasFlag() {
        verify(ParamUtils.hasFlag("--start-maximized", "--start-maximized"))
        verify(!ParamUtils.hasFlag("--new-window", "--start-maximized"))
        verify(!ParamUtils.hasFlag("", "--start-maximized"))
    }
}
