# SPDX-License-Identifier: MIT
cmake_minimum_required(VERSION 3.22)

project(webappstation VERSION 1.0.9 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(BUILD_TESTING "Build automated tests" ON)

find_package(ECM 6.0 REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(ECMFindQmlModule)
include(ECMQmlModule)

find_package(Qt6 REQUIRED COMPONENTS
    Core
    Gui
    Qml
    Quick
    QuickControls2
    Network
    Concurrent
    Widgets
)

if(BUILD_TESTING)
    find_package(Qt6 REQUIRED COMPONENTS Test QuickTest)
endif()

find_package(KF6 REQUIRED COMPONENTS
    Kirigami
    I18n
    CoreAddons
    Config
    IconThemes
)

ecm_find_qmlmodule(org.kde.kirigami REQUIRED)

add_subdirectory(src)

# Textos-fonte da UI em português (pt_BR). Traduções em po/<lang>/.
ki18n_install(po)

install(PROGRAMS org.kde.webappstation.desktop DESTINATION ${KDE_INSTALL_APPDIR})
install(FILES resources/icons/org.kde.webappstation.svg
        DESTINATION ${KDE_INSTALL_ICONDIR}/hicolor/scalable/apps)
install(DIRECTORY resources/firefox
        DESTINATION ${KDE_INSTALL_DATADIR}/webappstation)

if(BUILD_TESTING)
    enable_testing()
    add_subdirectory(autotests)
endif()

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)
