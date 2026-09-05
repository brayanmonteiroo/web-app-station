# SPDX-License-Identifier: MIT
FROM fedora:44
RUN dnf install -y cmake ninja-build gcc-c++ extra-cmake-modules \
    qt6-qtbase-devel qt6-qtdeclarative-devel \
    kf6-kirigami-devel kf6-ki18n-devel kf6-kcoreaddons-devel \
    kf6-kconfig-devel kf6-kiconthemes-devel kf6-qqc2-desktop-style \
    kf6-kirigami kf6-breeze-icons zsync curl \
    && dnf clean all
WORKDIR /src
