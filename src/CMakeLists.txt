# SPDX-License-Identifier: MIT
add_subdirectory(core)

add_library(webappstation-app STATIC
    AppController.cpp
    AppController.h
)

target_link_libraries(webappstation-app
    PUBLIC
        webappstation-core
        Qt6::Core
        Qt6::Gui
        Qt6::Qml
        KF6::I18n
        KF6::CoreAddons
)

target_compile_definitions(webappstation-app
    PUBLIC
        WEBAPPSTATION_VERSION="${PROJECT_VERSION}"
)

target_include_directories(webappstation-app
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/core
)

add_executable(webappstation)

ecm_add_qml_module(webappstation
    URI
    org.kde.webappstation
    GENERATE_PLUGIN_SOURCE
)

target_sources(webappstation
    PRIVATE
        main.cpp
)

ecm_target_qml_sources(webappstation
    SOURCES
        qml/Main.qml
        qml/ParamUtils.js
        qml/pages/WebAppListPage.qml
        qml/pages/WebAppEditorPage.qml
        qml/pages/FaviconPage.qml
        qml/pages/AboutPage.qml
        qml/pages/ShortcutsPage.qml
)

target_link_libraries(webappstation
    PRIVATE
        webappstation-app
        Qt6::Core
        Qt6::Gui
        Qt6::Qml
        Qt6::Quick
        Qt6::QuickControls2
        Qt6::Network
        Qt6::Concurrent
        Qt6::Widgets
        KF6::Kirigami
        KF6::I18n
        KF6::I18nQml
        KF6::CoreAddons
        KF6::ConfigCore
        KF6::IconThemes
)

target_compile_definitions(webappstation
    PRIVATE
        WEBAPPSTATION_VERSION="${PROJECT_VERSION}"
)

install(TARGETS webappstation ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
