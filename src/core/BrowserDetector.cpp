// SPDX-License-Identifier: MIT
#include "BrowserDetector.h"

#include <QDir>
#include <QFileInfo>

BrowserDetector::BrowserDetector(QObject *parent)
    : QObject(parent)
{
}

QList<Browser> BrowserDetector::allSupportedBrowsers() const
{
    using F = BrowserFamily;
    const QString home = QDir::homePath();

    return {
        {F::Firefox, QStringLiteral("Firefox"), QStringLiteral("firefox"),
         QStringLiteral("/usr/bin/firefox")},
        {F::Firefox, QStringLiteral("Firefox Developer Edition"),
         QStringLiteral("firefox-developer-edition"),
         QStringLiteral("/usr/bin/firefox-developer-edition")},
        {F::Firefox, QStringLiteral("Firefox Nightly"),
         QStringLiteral("firefox-nightly"),
         QStringLiteral("/usr/bin/firefox-nightly")},
        {F::Firefox, QStringLiteral("Firefox Extended Support Release"),
         QStringLiteral("firefox-esr"), QStringLiteral("/usr/bin/firefox-esr")},
        {F::FirefoxFlatpak, QStringLiteral("Firefox (Flatpak)"),
         QStringLiteral("/var/lib/flatpak/exports/bin/org.mozilla.firefox"),
         QStringLiteral("/var/lib/flatpak/exports/bin/org.mozilla.firefox")},
        {F::FirefoxFlatpak, QStringLiteral("Firefox (Flatpak)"),
         home
             + QStringLiteral(
                 "/.local/share/flatpak/exports/bin/org.mozilla.firefox"),
         home
             + QStringLiteral(
                 "/.local/share/flatpak/exports/bin/org.mozilla.firefox")},
        {F::ZenFlatpak, QStringLiteral("Zen (Flatpak)"),
         QStringLiteral("/var/lib/flatpak/exports/bin/app.zen_browser.zen"),
         QStringLiteral("/var/lib/flatpak/exports/bin/app.zen_browser.zen")},
        {F::ZenFlatpak, QStringLiteral("Zen (Flatpak)"),
         home
             + QStringLiteral(
                 "/.local/share/flatpak/exports/bin/app.zen_browser.zen"),
         home
             + QStringLiteral(
                 "/.local/share/flatpak/exports/bin/app.zen_browser.zen")},
        {F::FirefoxSnap, QStringLiteral("Firefox (Snap)"),
         QStringLiteral("/snap/bin/firefox"),
         QStringLiteral("/snap/bin/firefox")},
        {F::Chromium, QStringLiteral("Brave"), QStringLiteral("brave"),
         QStringLiteral("/usr/bin/brave")},
        {F::Chromium, QStringLiteral("Brave Browser"),
         QStringLiteral("brave-browser"),
         QStringLiteral("/usr/bin/brave-browser")},
        {F::Chromium, QStringLiteral("Brave (Bin)"), QStringLiteral("brave-bin"),
         QStringLiteral("/usr/bin/brave-bin")},
        {F::Chromium, QStringLiteral("Chrome"),
         QStringLiteral("google-chrome-stable"),
         QStringLiteral("/usr/bin/google-chrome-stable")},
        {F::Chromium, QStringLiteral("Chrome (Beta)"),
         QStringLiteral("google-chrome-beta"),
         QStringLiteral("/usr/bin/google-chrome-beta")},
        {F::Chromium, QStringLiteral("Chrome (Flatpak)"),
         QStringLiteral("/var/lib/flatpak/exports/bin/com.google.Chrome"),
         QStringLiteral("/var/lib/flatpak/exports/bin/com.google.Chrome")},
        {F::Chromium, QStringLiteral("Chrome (Flatpak)"),
         home
             + QStringLiteral(
                 "/.local/share/flatpak/exports/bin/com.google.Chrome"),
         home
             + QStringLiteral(
                 "/.local/share/flatpak/exports/bin/com.google.Chrome")},
        {F::Chromium, QStringLiteral("Chromium"), QStringLiteral("chromium"),
         QStringLiteral("/usr/bin/chromium")},
        {F::Chromium, QStringLiteral("Chromium (chromium-browser)"),
         QStringLiteral("chromium-browser"),
         QStringLiteral("/usr/bin/chromium-browser")},
        {F::Chromium, QStringLiteral("Chromium (Snap)"),
         QStringLiteral("chromium"), QStringLiteral("/snap/bin/chromium")},
        {F::Chromium, QStringLiteral("Chromium (Bin)"),
         QStringLiteral("chromium-bin"),
         QStringLiteral("/usr/bin/chromium-bin-browser")},
        {F::Chromium, QStringLiteral("Ungoogled Chromium"),
         QStringLiteral("ungoogled-chromium"),
         QStringLiteral("/usr/bin/ungoogled-chromium")},
        {F::Epiphany, QStringLiteral("Epiphany"), QStringLiteral("epiphany"),
         QStringLiteral("/usr/bin/epiphany")},
        {F::Firefox, QStringLiteral("LibreWolf"), QStringLiteral("librewolf"),
         QStringLiteral("/usr/bin/librewolf")},
        {F::LibreWolfFlatpak, QStringLiteral("LibreWolf (Flatpak)"),
         QStringLiteral(
             "/var/lib/flatpak/exports/bin/io.gitlab.librewolf-community"),
         QStringLiteral(
             "/var/lib/flatpak/exports/bin/io.gitlab.librewolf-community")},
        {F::LibreWolfFlatpak, QStringLiteral("LibreWolf (Flatpak)"),
         home
             + QStringLiteral(
                 "/.local/share/flatpak/exports/bin/"
                 "io.gitlab.librewolf-community"),
         home
             + QStringLiteral(
                 "/.local/share/flatpak/exports/bin/"
                 "io.gitlab.librewolf-community")},
        {F::Firefox, QStringLiteral("Waterfox"), QStringLiteral("waterfox"),
         QStringLiteral("/usr/bin/waterfox")},
        {F::Firefox, QStringLiteral("Waterfox Current"),
         QStringLiteral("waterfox-current"),
         QStringLiteral("/usr/bin/waterfox-current")},
        {F::Firefox, QStringLiteral("Waterfox Classic"),
         QStringLiteral("waterfox-classic"),
         QStringLiteral("/usr/bin/waterfox-classic")},
        {F::Firefox, QStringLiteral("Waterfox 3rd Generation"),
         QStringLiteral("waterfox-g3"), QStringLiteral("/usr/bin/waterfox-g3")},
        {F::Firefox, QStringLiteral("Waterfox 4th Generation"),
         QStringLiteral("waterfox-g4"), QStringLiteral("/usr/bin/waterfox-g4")},
        {F::Firefox, QStringLiteral("Floorp"), QStringLiteral("floorp"),
         QStringLiteral("/usr/bin/floorp")},
        {F::WaterfoxFlatpak, QStringLiteral("Waterfox (Flatpak)"),
         QStringLiteral("/var/lib/flatpak/exports/bin/net.waterfox.waterfox"),
         QStringLiteral("/var/lib/flatpak/exports/bin/net.waterfox.waterfox")},
        {F::WaterfoxFlatpak, QStringLiteral("Waterfox (Flatpak)"),
         home
             + QStringLiteral(
                 "/.local/share/flatpak/exports/bin/net.waterfox.waterfox"),
         home
             + QStringLiteral(
                 "/.local/share/flatpak/exports/bin/net.waterfox.waterfox")},
        {F::Chromium, QStringLiteral("Vivaldi"), QStringLiteral("vivaldi-stable"),
         QStringLiteral("/usr/bin/vivaldi-stable")},
        {F::Chromium, QStringLiteral("Vivaldi Snapshot"),
         QStringLiteral("vivaldi-snapshot"),
         QStringLiteral("/usr/bin/vivaldi-snapshot")},
        {F::Chromium, QStringLiteral("Vivaldi (Flatpak)"),
         QStringLiteral("/var/lib/flatpak/exports/bin/com.vivaldi.Vivaldi"),
         QStringLiteral("/var/lib/flatpak/exports/bin/com.vivaldi.Vivaldi")},
        {F::Chromium, QStringLiteral("Vivaldi (Flatpak)"),
         home
             + QStringLiteral(
                 "/.local/share/flatpak/exports/bin/com.vivaldi.Vivaldi"),
         home
             + QStringLiteral(
                 "/.local/share/flatpak/exports/bin/com.vivaldi.Vivaldi")},
        {F::Chromium, QStringLiteral("Microsoft Edge"),
         QStringLiteral("microsoft-edge-stable"),
         QStringLiteral("/usr/bin/microsoft-edge-stable")},
        {F::Chromium, QStringLiteral("Microsoft Edge Beta"),
         QStringLiteral("microsoft-edge-beta"),
         QStringLiteral("/usr/bin/microsoft-edge-beta")},
        {F::Chromium, QStringLiteral("Microsoft Edge Dev"),
         QStringLiteral("microsoft-edge-dev"),
         QStringLiteral("/usr/bin/microsoft-edge-dev")},
        {F::Chromium, QStringLiteral("FlashPeak Slimjet"),
         QStringLiteral("flashpeak-slimjet"),
         QStringLiteral("/usr/bin/flashpeak-slimjet")},
        {F::Chromium, QStringLiteral("Ungoogled Chromium (Flatpak)"),
         QStringLiteral(
             "/var/lib/flatpak/exports/bin/"
             "io.github.ungoogled_software.ungoogled_chromium"),
         QStringLiteral(
             "/var/lib/flatpak/exports/bin/"
             "io.github.ungoogled_software.ungoogled_chromium")},
        {F::Chromium, QStringLiteral("Ungoogled Chromium (Flatpak)"),
         home
             + QStringLiteral(
                 "/.local/share/flatpak/exports/bin/"
                 "io.github.ungoogled_software.ungoogled_chromium"),
         home
             + QStringLiteral(
                 "/.local/share/flatpak/exports/bin/"
                 "io.github.ungoogled_software.ungoogled_chromium")},
        {F::Chromium, QStringLiteral("Chromium (Flatpak)"),
         QStringLiteral("/var/lib/flatpak/exports/bin/org.chromium.Chromium"),
         QStringLiteral("/var/lib/flatpak/exports/bin/org.chromium.Chromium")},
        {F::Chromium, QStringLiteral("Chromium (Flatpak)"),
         home
             + QStringLiteral(
                 "/.local/share/flatpak/exports/bin/org.chromium.Chromium"),
         home
             + QStringLiteral(
                 "/.local/share/flatpak/exports/bin/org.chromium.Chromium")},
        {F::Falkon, QStringLiteral("Falkon"), QStringLiteral("falkon"),
         QStringLiteral("/usr/bin/falkon")},
        {F::Chromium, QStringLiteral("Edge (Flatpak)"),
         QStringLiteral("/var/lib/flatpak/exports/bin/com.microsoft.Edge"),
         QStringLiteral("/var/lib/flatpak/exports/bin/com.microsoft.Edge")},
        {F::Chromium, QStringLiteral("Edge (Flatpak)"),
         home
             + QStringLiteral(
                 "/.local/share/flatpak/exports/bin/com.microsoft.Edge"),
         home
             + QStringLiteral(
                 "/.local/share/flatpak/exports/bin/com.microsoft.Edge")},
        {F::Chromium, QStringLiteral("Brave (Flatpak)"),
         QStringLiteral("/var/lib/flatpak/exports/bin/com.brave.Browser"),
         QStringLiteral("/var/lib/flatpak/exports/bin/com.brave.Browser")},
        {F::Chromium, QStringLiteral("Brave (Flatpak)"),
         home
             + QStringLiteral(
                 "/.local/share/flatpak/exports/bin/com.brave.Browser"),
         home
             + QStringLiteral(
                 "/.local/share/flatpak/exports/bin/com.brave.Browser")},
        {F::Chromium, QStringLiteral("Yandex"), QStringLiteral("yandex-browser"),
         QStringLiteral("/usr/bin/yandex-browser")},
        {F::Falkon, QStringLiteral("Falkon (Flatpak)"),
         QStringLiteral("/var/lib/flatpak/exports/bin/org.kde.falkon"),
         QStringLiteral("/var/lib/flatpak/exports/bin/org.kde.falkon")},
        {F::Falkon, QStringLiteral("Falkon (Flatpak)"),
         home
             + QStringLiteral(
                 "/.local/share/flatpak/exports/bin/org.kde.falkon"),
         home
             + QStringLiteral(
                 "/.local/share/flatpak/exports/bin/org.kde.falkon")},
        {F::Chromium, QStringLiteral("Naver Whale"),
         QStringLiteral("naver-whale-stable"),
         QStringLiteral("/usr/bin/naver-whale-stable")},
        {F::Chromium, QStringLiteral("Yandex (Flatpak)"),
         QStringLiteral("/var/lib/flatpak/exports/bin/ru.yandex.Browser"),
         QStringLiteral("/var/lib/flatpak/exports/bin/ru.yandex.Browser")},
        {F::Chromium, QStringLiteral("Yandex (Flatpak)"),
         home
             + QStringLiteral(
                 "/.local/share/flatpak/exports/bin/ru.yandex.Browser"),
         home
             + QStringLiteral(
                 "/.local/share/flatpak/exports/bin/ru.yandex.Browser")},
        {F::Chromium, QStringLiteral("Thorium"),
         QStringLiteral("thorium-browser"),
         QStringLiteral("/usr/bin/thorium-browser")},
        {F::FloorpFlatpak, QStringLiteral("Floorp (Flatpak)"),
         QStringLiteral("/var/lib/flatpak/exports/bin/one.ablaze.floorp"),
         QStringLiteral("/var/lib/flatpak/exports/bin/one.ablaze.floorp")},
        {F::FloorpFlatpak, QStringLiteral("Floorp (Flatpak)"),
         home
             + QStringLiteral(
                 "/.local/share/flatpak/exports/bin/one.ablaze.floorp"),
         home
             + QStringLiteral(
                 "/.local/share/flatpak/exports/bin/one.ablaze.floorp")},
    };
}

QList<Browser> BrowserDetector::installedBrowsers() const
{
    QList<Browser> result;
    QStringList seenNames;
    for (const Browser &browser : allSupportedBrowsers()) {
        if (!QFileInfo::exists(browser.testPath)) {
            continue;
        }
        if (seenNames.contains(browser.name)) {
            continue;
        }
        seenNames.append(browser.name);
        result.append(browser);
    }
    return result;
}

Browser BrowserDetector::findByName(const QString &name) const
{
    for (const Browser &browser : installedBrowsers()) {
        if (browser.name == name) {
            return browser;
        }
    }
    for (const Browser &browser : allSupportedBrowsers()) {
        if (browser.name == name && QFileInfo::exists(browser.testPath)) {
            return browser;
        }
    }
    return {};
}
