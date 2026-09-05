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
TOOLS_DIR="$ROOT/.appimage-tools"

DESKTOP="$ROOT/org.kde.webappstation.desktop"
ICON="$ROOT/resources/icons/org.kde.webappstation.svg"

download_tool() {
  local url="$1"
  local dest="$2"
  if [[ ! -x "$dest" ]]; then
    echo "==> Baixando $(basename "$dest")..."
    curl -fsSL "$url" -o "$dest"
    chmod +x "$dest"
  fi
}

extract_appimage() {
  local ai="$1"
  local dest="$2"
  rm -rf "$dest"
  mkdir -p "$dest"
  (
    cd "$dest"
    "$ai" --appimage-extract >/dev/null
  )
}

echo "==> Configurando build..."
cmake -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX=/usr

echo "==> Compilando..."
cmake --build build

echo "==> Instalando em AppDir..."
rm -rf "$APPDIR"
DESTDIR="$APPDIR" cmake --install build

mkdir -p "$APPDIR/usr/share/applications" "$APPDIR/usr/share/icons/hicolor/scalable/apps"
cp -f "$DESKTOP" "$APPDIR/usr/share/applications/"
cp -f "$ICON" "$APPDIR/usr/share/icons/hicolor/scalable/apps/${APP_ID}.svg"
ln -sfn "usr/share/applications/${APP_ID}.desktop" "$APPDIR/${APP_ID}.desktop"
ln -sfn "usr/share/icons/hicolor/scalable/apps/${APP_ID}.svg" "$APPDIR/${APP_ID}.svg"
ln -sfn "usr/share/icons/hicolor/scalable/apps/${APP_ID}.svg" "$APPDIR/.DirIcon"

mkdir -p "$APPDIR/usr/share/webappstation"
cp -a "$ROOT/resources/firefox" "$APPDIR/usr/share/webappstation/"

download_tool \
  "https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage" \
  "$LINUXDEPLOY"
download_tool \
  "https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage" \
  "$LINUXDEPLOY_QT"
download_tool \
  "https://github.com/AppImage/appimagetool/releases/download/continuous/appimagetool-x86_64.AppImage" \
  "$APPIMAGETOOL"

# Extrai ferramentas (mais confiável que FUSE aninhado no CI).
mkdir -p "$TOOLS_DIR"
extract_appimage "$LINUXDEPLOY" "$TOOLS_DIR/linuxdeploy"
extract_appimage "$LINUXDEPLOY_QT" "$TOOLS_DIR/linuxdeploy-qt"
extract_appimage "$APPIMAGETOOL" "$TOOLS_DIR/appimagetool"

# strip embutido é binutils antigo (não lê .relr.dyn do Fedora/glibc moderno).
# Preferir strip do sistema; se ausente, NO_STRIP desliga o passo.
rm -f "$TOOLS_DIR/linuxdeploy/squashfs-root/usr/bin/strip" \
      "$TOOLS_DIR/linuxdeploy-qt/squashfs-root/usr/bin/strip" || true
export NO_STRIP="${NO_STRIP:-1}"
# Em containers sem FUSE, AppImages filhos falham com 127.
export APPIMAGE_EXTRACT_AND_RUN=1

LINUXDEPLOY_BIN="$TOOLS_DIR/linuxdeploy/squashfs-root/AppRun"
# Plugin precisa estar no PATH com nome linuxdeploy-plugin-qt
PLUGIN_DIR="$TOOLS_DIR/linuxdeploy-qt/squashfs-root"
if [[ -x "$PLUGIN_DIR/AppRun" ]]; then
  ln -sfn "$PLUGIN_DIR/AppRun" "$TOOLS_DIR/linuxdeploy-plugin-qt"
fi
export PATH="$TOOLS_DIR:$PATH"
APPIMAGETOOL_BIN="$TOOLS_DIR/appimagetool/squashfs-root/AppRun"

# linuxdeploy prioriza *.AppImage no CWD sobre o plugin extraído no PATH.
# Sem FUSE no CI isso dá exit 127 em --plugin-api-version.
chmod a-x "$LINUXDEPLOY" "$LINUXDEPLOY_QT" "$APPIMAGETOOL" 2>/dev/null || true

if ! "$TOOLS_DIR/linuxdeploy-plugin-qt" --plugin-api-version >/dev/null 2>&1; then
  echo "Erro: linuxdeploy-plugin-qt extraído não responde a --plugin-api-version." >&2
  "$TOOLS_DIR/linuxdeploy-plugin-qt" --plugin-api-version || true
  exit 1
fi
echo "==> Plugin Qt OK (extraído): $(command -v linuxdeploy-plugin-qt)"

QMAKE_BIN="$(command -v qmake6 || command -v qmake-qt6 || command -v qmake || true)"
if [[ -z "$QMAKE_BIN" ]]; then
  echo "Erro: qmake6 não encontrado (necessário para linuxdeploy-plugin-qt)." >&2
  exit 1
fi
export QMAKE="$QMAKE_BIN"
export QML_SOURCES_PATHS="$ROOT/src/qml"
export EXTRA_QT_PLUGINS="wayland;xcb;platforms;platformthemes;imageformats;iconengines;tls;networkinformation;generic"

# Nomes de plugins variam entre distros/Qt (Fedora: libqwayland.so; alguns: *-generic/-egl).
QT_PLATFORMS_DIR=""
for d in /usr/lib64/qt6/plugins/platforms /usr/lib/qt6/plugins/platforms /usr/lib/x86_64-linux-gnu/qt6/plugins/platforms; do
  if [[ -d "$d" ]]; then
    QT_PLATFORMS_DIR="$d"
    break
  fi
done
PLATFORM_PLUGINS=()
for plugin in libqxcb.so libqwayland.so libqwayland-generic.so libqwayland-egl.so; do
  if [[ -n "$QT_PLATFORMS_DIR" && -f "$QT_PLATFORMS_DIR/$plugin" ]]; then
    PLATFORM_PLUGINS+=("$plugin")
  fi
done
if [[ ${#PLATFORM_PLUGINS[@]} -gt 0 ]]; then
  export EXTRA_PLATFORM_PLUGINS
  EXTRA_PLATFORM_PLUGINS="$(IFS=';'; echo "${PLATFORM_PLUGINS[*]}")"
  echo "==> Platform plugins: $EXTRA_PLATFORM_PLUGINS"
fi

echo "==> Empacotando dependências Qt/KF com linuxdeploy (qmake=$QMAKE)..."
"$LINUXDEPLOY_BIN" \
  --appdir "$APPDIR" \
  --executable "$APPDIR/usr/bin/${BINARY}" \
  --desktop-file "$APPDIR/usr/share/applications/${APP_ID}.desktop" \
  --icon-file "$APPDIR/usr/share/icons/hicolor/scalable/apps/${APP_ID}.svg" \
  --plugin qt

# appimageupdatetool DEPOIS do linuxdeploy — senão ele tenta resolver
# libappimageupdate.so e aborta o deploy.
UPDATE_TOOL_URL="https://github.com/AppImageCommunity/AppImageUpdate/releases/download/continuous/appimageupdatetool-x86_64.AppImage"
UPDATE_TOOL_AI="$ROOT/appimageupdatetool-x86_64.AppImage"
UPDATE_TOOL_DEST="$APPDIR/usr/bin/appimageupdatetool"
mkdir -p "$APPDIR/usr/bin" "$APPDIR/usr/lib"
if [[ ! -x "$UPDATE_TOOL_DEST" ]]; then
  echo "==> Baixando e extraindo appimageupdatetool..."
  download_tool "$UPDATE_TOOL_URL" "$UPDATE_TOOL_AI"
  EXTRACT_DIR="$ROOT/.appimageupdatetool-extract"
  extract_appimage "$UPDATE_TOOL_AI" "$EXTRACT_DIR"
  FOUND="$(find "$EXTRACT_DIR/squashfs-root" -type f -name 'appimageupdatetool' 2>/dev/null | head -1 || true)"
  if [[ -n "$FOUND" && -f "$FOUND" ]]; then
    cp -f "$FOUND" "$UPDATE_TOOL_DEST"
    chmod +x "$UPDATE_TOOL_DEST"
    # Libs do updater (não passar pelo linuxdeploy).
    find "$EXTRACT_DIR/squashfs-root" -name 'libappimageupdate*.so*' -exec cp -a {} "$APPDIR/usr/lib/" \; || true
  else
    echo "Aviso: extraindo appimageupdatetool falhou; updates in-app podem não funcionar." >&2
  fi
  rm -rf "$EXTRACT_DIR"
fi

# libxcb-* empacotado + libxcb.so do sistema = SIGSEGV em dl_init.
# Preferir helpers X11/xcb do host (linuxdeploy já blacklist libxcb.so.1).
rm -f "$APPDIR"/usr/lib/libxcb-*.so* || true

copy_glob() {
  local pattern="$1"
  local dest="$2"
  mkdir -p "$dest"
  # shellcheck disable=SC2086
  for f in $pattern; do
    [[ -e "$f" ]] || continue
    cp -a "$f" "$dest/"
  done
}

# KF6 / Kirigami (além do que o plugin Qt puxar)
copy_glob "/usr/lib64/libKF6*.so*" "$APPDIR/usr/lib"
copy_glob "/usr/lib64/libKirigami*.so*" "$APPDIR/usr/lib"
copy_glob "/usr/lib64/libkirigami*.so*" "$APPDIR/usr/lib"

# QML org.kde.* (Kirigami, desktop style, etc.)
mkdir -p "$APPDIR/usr/qml/org" "$APPDIR/usr/lib/qml/org"
for qmlroot in /usr/lib64/qt6/qml /usr/lib64/qml /usr/lib/qt6/qml; do
  if [[ -d "$qmlroot/org/kde" ]]; then
    cp -a "$qmlroot/org/kde" "$APPDIR/usr/qml/org/"
    cp -a "$qmlroot/org/kde" "$APPDIR/usr/lib/qml/org/"
  fi
  # QtQuick modules que Kirigami precisa
  for mod in QtQuick QtQml Qt; do
    if [[ -d "$qmlroot/$mod" ]]; then
      mkdir -p "$APPDIR/usr/qml"
      cp -a "$qmlroot/$mod" "$APPDIR/usr/qml/" || true
    fi
  done
done

# Fallback: se linuxdeploy não empacotou Qt, copia à força.
if ! find "$APPDIR" -name 'libQt6Core.so*' | grep -q .; then
  echo "==> Fallback: copiando libQt6*.so do sistema..."
  copy_glob "/usr/lib64/libQt6*.so*" "$APPDIR/usr/lib"
fi

if ! find "$APPDIR" -name 'libQt6Core.so*' | grep -q .; then
  echo "Erro: libQt6Core não está no AppDir — AppImage quebraria no host." >&2
  exit 1
fi

# Plugins Qt (platform/wayland/xcb) se ainda faltarem
for plugdir in /usr/lib64/qt6/plugins /usr/lib/qt6/plugins; do
  if [[ -d "$plugdir" ]]; then
    mkdir -p "$APPDIR/usr/plugins"
    cp -a "$plugdir/." "$APPDIR/usr/plugins/" || true
  fi
done

# linuxdeploy deixa AppRun -> usr/bin/webappstation; escrever sem rm
# seguiria o symlink e sobrescreveria o binário com este script.
rm -f "$APPDIR/AppRun"
cat > "$APPDIR/AppRun" << 'EOF'
#!/bin/bash
HERE="$(dirname "$(readlink -f "$0")")"
export PATH="${HERE}/usr/bin:${PATH}"
# NÃO exportar LD_LIBRARY_PATH: o binário já tem RPATH ($ORIGIN/../lib) e
# qt.conf. Forçar usr/lib no LD_LIBRARY_PATH mistura libs e causa SIGSEGV
# ao carregar plugins QML (ex.: Controls/Basic).
export QT_PLUGIN_PATH="${HERE}/usr/plugins"
export QML2_IMPORT_PATH="${HERE}/usr/qml:${HERE}/usr/lib/qml"
export QML_IMPORT_PATH="$QML2_IMPORT_PATH"
export XDG_DATA_DIRS="${HERE}/usr/share:/usr/share:/usr/local/share${XDG_DATA_DIRS:+:$XDG_DATA_DIRS}"
unset QT_ROOT_PATH
# Evitar que o host injete outro Qt via LD_LIBRARY_PATH do usuário.
unset LD_LIBRARY_PATH
exec "${HERE}/usr/bin/webappstation" "$@"
EOF
chmod +x "$APPDIR/AppRun"

UPDATE_INFO="gh-releases-zsync|brayanmonteiroo|web-app-station|latest|WebAppStation-*-x86_64.AppImage.zsync"
ZSYNC_OUTPUT="${OUTPUT}.zsync"

echo "==> Gerando AppImage com update information..."
rm -f "$OUTPUT" "$ZSYNC_OUTPUT"
ARCH=x86_64 "$APPIMAGETOOL_BIN" \
  -u "$UPDATE_INFO" \
  "$APPDIR" \
  "$OUTPUT"

if [[ ! -f "$OUTPUT" ]]; then
  echo "Erro: AppImage não foi gerado." >&2
  exit 1
fi

# appimagetool -u gera .zsync; fallback com zsyncmake se disponível.
if [[ ! -f "$ZSYNC_OUTPUT" ]] && command -v zsyncmake >/dev/null 2>&1; then
  echo "==> Gerando .zsync com zsyncmake..."
  zsyncmake -u "$(basename "$OUTPUT")" -o "$ZSYNC_OUTPUT" "$OUTPUT"
fi

if [[ ! -f "$ZSYNC_OUTPUT" ]]; then
  echo "Aviso: .zsync não gerado (updates via AppImageUpdate podem falhar)." >&2
fi

# Sanity: AppImage deve ser bem maior que ~14MB se Qt foi empacotado.
SIZE_BYTES="$(stat -c%s "$OUTPUT" 2>/dev/null || stat -f%z "$OUTPUT")"
if [[ "$SIZE_BYTES" -lt 40000000 ]]; then
  echo "Erro: AppImage muito pequeno (${SIZE_BYTES} bytes) — Qt provavelmente não foi empacotado." >&2
  exit 1
fi

chmod +x "$OUTPUT"
echo "==> Pronto: $OUTPUT ($(du -h "$OUTPUT" | cut -f1))"
echo "    Zsync:  $ZSYNC_OUTPUT"
