#!/usr/bin/env bash
# Build Web App Station AppImage (Qt6 + KF6 bundled via linuxdeploy).
# Update info mirrors FormatPen: gh-releases-zsync + .zsync artifact.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT"

APP_ID="org.kde.webappstation"
BINARY="webappstation"
APPDIR="$ROOT/AppDir"
VERSION="$(grep -E 'project\(webappstation VERSION' CMakeLists.txt | sed -E 's/.*VERSION ([0-9.]+).*/\1/')"
OUTPUT="$ROOT/WebAppStation-${VERSION}-x86_64.AppImage"
APPIMAGETOOL="${APPIMAGETOOL:-$ROOT/appimagetool}"
LINUXDEPLOY="${LINUXDEPLOY:-$ROOT/linuxdeploy-x86_64.AppImage}"
LINUXDEPLOY_QT="${LINUXDEPLOY_QT:-$ROOT/linuxdeploy-plugin-qt-x86_64.AppImage}"

DESKTOP="$ROOT/org.kde.webappstation.desktop"
ICON="$ROOT/resources/icons/org.kde.webappstation.svg"

echo "==> Configurando build..."
cmake -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX=/usr

echo "==> Compilando..."
cmake --build build

echo "==> Instalando em AppDir..."
rm -rf "$APPDIR"
DESTDIR="$APPDIR" cmake --install build

# Ensure desktop/icon at AppDir root for appimagetool
mkdir -p "$APPDIR/usr/share/applications" "$APPDIR/usr/share/icons/hicolor/scalable/apps"
cp -f "$DESKTOP" "$APPDIR/usr/share/applications/"
cp -f "$ICON" "$APPDIR/usr/share/icons/hicolor/scalable/apps/${APP_ID}.svg"
ln -sfn "usr/share/applications/${APP_ID}.desktop" "$APPDIR/${APP_ID}.desktop"
ln -sfn "usr/share/icons/hicolor/scalable/apps/${APP_ID}.svg" "$APPDIR/${APP_ID}.svg"
ln -sfn "usr/share/icons/hicolor/scalable/apps/${APP_ID}.svg" "$APPDIR/.DirIcon"

# Copy firefox templates if install layout differs
mkdir -p "$APPDIR/usr/share/webappstation"
cp -a "$ROOT/resources/firefox" "$APPDIR/usr/share/webappstation/"

download_tool() {
  local url="$1"
  local dest="$2"
  if [[ ! -x "$dest" ]]; then
    echo "==> Baixando $(basename "$dest")..."
    curl -sSL "$url" -o "$dest"
    chmod +x "$dest"
  fi
}

download_tool \
  "https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage" \
  "$LINUXDEPLOY"
download_tool \
  "https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage" \
  "$LINUXDEPLOY_QT"
download_tool \
  "https://github.com/AppImage/appimagetool/releases/download/continuous/appimagetool-x86_64.AppImage" \
  "$APPIMAGETOOL"

# Optional AppImageUpdate tool for in-app updates
UPDATE_TOOL_URL="https://github.com/AppImageCommunity/AppImageUpdate/releases/download/continuous/appimageupdatetool-x86_64.AppImage"
UPDATE_TOOL_DEST="$APPDIR/usr/bin/appimageupdatetool"
if [[ ! -f "$UPDATE_TOOL_DEST" ]]; then
  echo "==> Baixando appimageupdatetool..."
  curl -sSL "$UPDATE_TOOL_URL" -o "$UPDATE_TOOL_DEST" || true
  chmod +x "$UPDATE_TOOL_DEST" 2>/dev/null || true
fi

echo "==> Empacotando dependências Qt/KF com linuxdeploy..."
export QML_SOURCES_PATHS="$ROOT/src/qml"
export EXTRA_QT_PLUGINS="wayland;xcb"
# Bundle non-Qt KF libs discovered from the binary
"$LINUXDEPLOY" --appimage-extract-and-run \
  --appdir "$APPDIR" \
  --executable "$APPDIR/usr/bin/${BINARY}" \
  --desktop-file "$APPDIR/usr/share/applications/${APP_ID}.desktop" \
  --icon-file "$APPDIR/usr/share/icons/hicolor/scalable/apps/${APP_ID}.svg" \
  --plugin qt \
  || true

# Copy common KF6/Kirigami runtime pieces from the build host
copy_glob() {
  local pattern="$1"
  local dest="$2"
  mkdir -p "$dest"
  # shellcheck disable=SC2086
  for f in $pattern; do
    [[ -e "$f" ]] || continue
    cp -a "$f" "$dest/" || true
  done
}

copy_glob "/usr/lib64/libKF6*.so*" "$APPDIR/usr/lib"
copy_glob "/usr/lib64/libKirigami*.so*" "$APPDIR/usr/lib"
mkdir -p "$APPDIR/usr/lib/qml"
if [[ -d /usr/lib64/qt6/qml/org/kde ]]; then
  mkdir -p "$APPDIR/usr/lib/qml/org"
  cp -a /usr/lib64/qt6/qml/org/kde "$APPDIR/usr/lib/qml/org/" || true
fi
if [[ -d /usr/lib64/qml/org/kde ]]; then
  mkdir -p "$APPDIR/usr/lib/qml/org"
  cp -a /usr/lib64/qml/org/kde "$APPDIR/usr/lib/qml/org/" || true
fi

cat > "$APPDIR/AppRun" << 'EOF'
#!/bin/bash
HERE="$(dirname "$(readlink -f "$0")")"
export PATH="${HERE}/usr/bin:${PATH}"
export LD_LIBRARY_PATH="${HERE}/usr/lib:${HERE}/usr/lib64:${LD_LIBRARY_PATH:-}"
export QT_PLUGIN_PATH="${HERE}/usr/plugins:${HERE}/usr/lib/qt6/plugins:${QT_PLUGIN_PATH:-}"
export QML2_IMPORT_PATH="${HERE}/usr/qml:${HERE}/usr/lib/qml:${HERE}/usr/lib64/qml:${QML2_IMPORT_PATH:-}"
export XDG_DATA_DIRS="${HERE}/usr/share:/usr/share:/usr/local/share:${XDG_DATA_DIRS:-/usr/share}"
exec "${HERE}/usr/bin/webappstation" "$@"
EOF
chmod +x "$APPDIR/AppRun"

if ! command -v zsyncmake >/dev/null 2>&1; then
  echo "Erro: zsyncmake não encontrado (instale o pacote zsync)." >&2
  exit 1
fi

UPDATE_INFO="gh-releases-zsync|brayanmonteiroo|web-app-station|latest|WebAppStation-*-x86_64.AppImage.zsync"
ZSYNC_OUTPUT="${OUTPUT}.zsync"

echo "==> Gerando AppImage com update information..."
rm -f "$OUTPUT" "$ZSYNC_OUTPUT"
ARCH=x86_64 "$APPIMAGETOOL" --appimage-extract-and-run \
  -u "$UPDATE_INFO" \
  "$APPDIR" \
  "$OUTPUT"

if [[ ! -f "$OUTPUT" ]]; then
  echo "Erro: AppImage não foi gerado." >&2
  exit 1
fi

if [[ ! -f "$ZSYNC_OUTPUT" ]]; then
  echo "Erro: arquivo .zsync não foi gerado." >&2
  exit 1
fi

chmod +x "$OUTPUT"
echo "==> Pronto: $OUTPUT"
echo "    Zsync:  $ZSYNC_OUTPUT"
