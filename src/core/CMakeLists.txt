# SPDX-License-Identifier: MIT
add_library(webappstation-core STATIC
    Browser.cpp
    Browser.h
    BrowserDetector.cpp
    BrowserDetector.h
    DesktopEntry.cpp
    DesktopEntry.h
    ExecBuilder.cpp
    ExecBuilder.h
    FaviconService.cpp
    FaviconService.h
    LocaleService.cpp
    LocaleService.h
    Paths.h
    ProfileService.cpp
    ProfileService.h
    UpdateService.cpp
    UpdateService.h
    WebApp.cpp
    WebApp.h
    WebAppManager.cpp
    WebAppManager.h
)

target_include_directories(webappstation-core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(webappstation-core
    PUBLIC
        Qt6::Core
        Qt6::Gui
        Qt6::Network
        Qt6::Concurrent
        KF6::CoreAddons
        KF6::ConfigCore
        KF6::I18n
)
