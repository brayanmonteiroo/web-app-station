# SPDX-License-Identifier: MIT

set(AUTOTESTS_COMMON_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR})

function(webappstation_add_test name)
    add_executable(${name} ${ARGN})
    target_include_directories(${name} PRIVATE ${AUTOTESTS_COMMON_INCLUDE})
    target_link_libraries(${name}
        PRIVATE
            webappstation-core
            Qt6::Core
            Qt6::Gui
            Qt6::Test
            Qt6::Network
            Qt6::Concurrent
            KF6::I18n
            KF6::CoreAddons
            KF6::ConfigCore
    )
    add_test(NAME ${name} COMMAND ${name})
    set_tests_properties(${name} PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
endfunction()

webappstation_add_test(tst_desktopentry tst_desktopentry.cpp)
webappstation_add_test(tst_execbuilder tst_execbuilder.cpp)
webappstation_add_test(tst_webappmanager tst_webappmanager.cpp)
webappstation_add_test(tst_firefoxprofile tst_firefoxprofile.cpp)
webappstation_add_test(tst_windowicon tst_windowicon.cpp)

add_executable(tst_qmlsources tst_qmlsources.cpp)
target_compile_definitions(tst_qmlsources
    PRIVATE SOURCE_DIR="${CMAKE_SOURCE_DIR}")
target_link_libraries(tst_qmlsources PRIVATE Qt6::Core Qt6::Test)
add_test(NAME tst_qmlsources COMMAND tst_qmlsources)
set_tests_properties(tst_qmlsources PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

add_executable(tst_appcontroller tst_appcontroller.cpp)
target_include_directories(tst_appcontroller PRIVATE ${AUTOTESTS_COMMON_INCLUDE})
target_link_libraries(tst_appcontroller
    PRIVATE
        webappstation-app
        Qt6::Core
        Qt6::Gui
        Qt6::Test
        Qt6::Network
        Qt6::Concurrent
        KF6::I18n
        KF6::CoreAddons
        KF6::ConfigCore
)
add_test(NAME tst_appcontroller COMMAND tst_appcontroller)
set_tests_properties(tst_appcontroller PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# Qt Quick Test — ParamUtils + smoke
add_executable(tst_qml
    qml/tst_setup.cpp
    qml/tst_toggle_param.qml
    qml/tst_about_smoke.qml
)
target_compile_definitions(tst_qml
    PRIVATE
        QUICK_TEST_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/qml"
)
target_link_libraries(tst_qml
    PRIVATE
        Qt6::QuickTest
        Qt6::Qml
        Qt6::Quick
        Qt6::Gui
)
add_test(NAME tst_qml COMMAND tst_qml -input ${CMAKE_CURRENT_SOURCE_DIR}/qml)
set_tests_properties(tst_qml PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
