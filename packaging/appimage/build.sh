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
ICON_PNG="$ROOT/resources/icons/org.kde.webappstation.png"


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

mkdir -p "$APPDIR/usr/share/applications" \
  "$APPDIR/usr/share/icons/hicolor/scalable/apps" \
  "$APPDIR/usr/share/icons/hicolor/256x256/apps"
cp -f "$DESKTOP" "$APPDIR/usr/share/applications/"
cp -f "$ICON" "$APPDIR/usr/share/icons/hicolor/scalable/apps/${APP_ID}.svg"
if [[ -f "$ICON_PNG" ]]; then
  cp -f "$ICON_PNG" "$APPDIR/usr/share/icons/hicolor/256x256/apps/${APP_ID}.png"
fi
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
echo "==> Baixando e extraindo appimageupdatetool..."
download_tool "$UPDATE_TOOL_URL" "$UPDATE_TOOL_AI"
EXTRACT_DIR="$ROOT/.appimageupdatetool-extract"
extract_appimage "$UPDATE_TOOL_AI" "$EXTRACT_DIR"
FOUND="$(find "$EXTRACT_DIR/squashfs-root" -type f -name 'appimageupdatetool' 2>/dev/null | head -1 || true)"
if [[ -z "$FOUND" || ! -f "$FOUND" ]]; then
  echo "Erro: appimageupdatetool não encontrado no AppImage do updater." >&2
  rm -rf "$EXTRACT_DIR"
  exit 1
fi
cp -f "$FOUND" "$UPDATE_TOOL_DEST"
chmod +x "$UPDATE_TOOL_DEST"
# Libs do updater (OpenSSL 1.1, gpgme, …) em subpasta — não misturar com Qt6/OpenSSL3.
UPDATE_LIB_DIR="$APPDIR/usr/lib/appimageupdate"
rm -rf "$UPDATE_LIB_DIR"
mkdir -p "$UPDATE_LIB_DIR"
if [[ -d "$EXTRACT_DIR/squashfs-root/usr/lib" ]]; then
  cp -a "$EXTRACT_DIR/squashfs-root/usr/lib/." "$UPDATE_LIB_DIR/"
fi
# Algumas builds usam multiarch.
if [[ -d "$EXTRACT_DIR/squashfs-root/usr/lib/x86_64-linux-gnu" ]]; then
  cp -a "$EXTRACT_DIR/squashfs-root/usr/lib/x86_64-linux-gnu/." "$UPDATE_LIB_DIR/"
fi
rm -rf "$EXTRACT_DIR"
if [[ ! -x "$UPDATE_TOOL_DEST" ]]; then
  echo "Erro: falha ao instalar appimageupdatetool em $UPDATE_TOOL_DEST." >&2
  exit 1
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

# KF6 / Kirigami (além do que o plugin Qt puxar).
# Só copia o que ainda falta — não sobrescrever libs já patchelfadas pelo linuxdeploy.
copy_missing_glob() {
  local pattern="$1"
  local dest="$2"
  mkdir -p "$dest"
  # shellcheck disable=SC2086
  for f in $pattern; do
    [[ -e "$f" ]] || continue
    local base
    base="$(basename "$f")"
    if [[ -e "$dest/$base" ]]; then
      continue
    fi
    cp -a "$f" "$dest/"
  done
}

copy_missing_glob "/usr/lib64/libKF6*.so*" "$APPDIR/usr/lib"
copy_missing_glob "/usr/lib64/libKirigami*.so*" "$APPDIR/usr/lib"
copy_missing_glob "/usr/lib64/libkirigami*.so*" "$APPDIR/usr/lib"

# QML org.kde.* (Kirigami) — sem qqc2-desktop-style (estilo Plasma).
mkdir -p "$APPDIR/usr/qml/org" "$APPDIR/usr/lib/qml/org"
for qmlroot in /usr/lib64/qt6/qml /usr/lib64/qml /usr/lib/qt6/qml; do
  if [[ -d "$qmlroot/org/kde" ]]; then
    cp -a "$qmlroot/org/kde" "$APPDIR/usr/qml/org/"
    cp -a "$qmlroot/org/kde" "$APPDIR/usr/lib/qml/org/"
  fi
  for mod in QtQuick QtQml Qt; do
    if [[ -d "$qmlroot/$mod" ]]; then
      mkdir -p "$APPDIR/usr/qml"
      cp -a "$qmlroot/$mod" "$APPDIR/usr/qml/" || true
    fi
  done
done
rm -rf "$APPDIR/usr/qml/org/kde/qqc2desktopstyle" \
       "$APPDIR/usr/lib/qml/org/kde/qqc2desktopstyle" || true

# Ícones mínimos no AppDir (não depender do tema do DE do host).
for icondir in /usr/share/icons/breeze /usr/share/icons/breeze-dark \
               /usr/share/icons/hicolor; do
  if [[ -d "$icondir" ]]; then
    base="$(basename "$icondir")"
    mkdir -p "$APPDIR/usr/share/icons"
    if [[ ! -d "$APPDIR/usr/share/icons/$base" ]]; then
      echo "==> Copiando ícones $base..."
      cp -a "$icondir" "$APPDIR/usr/share/icons/" || true
    fi
  fi
done
# Garante o ícone do app mesmo se hicolor do sistema sobrescrever parcialmente.
mkdir -p "$APPDIR/usr/share/icons/hicolor/scalable/apps" \
  "$APPDIR/usr/share/icons/hicolor/256x256/apps"
cp -f "$ICON" "$APPDIR/usr/share/icons/hicolor/scalable/apps/${APP_ID}.svg"
if [[ -f "$ICON_PNG" ]]; then
  cp -f "$ICON_PNG" "$APPDIR/usr/share/icons/hicolor/256x256/apps/${APP_ID}.png"
fi

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

# AppImage self-contained: LD_LIBRARY_PATH no AppRun = só ${HERE}/usr/lib.
# NÃO rodar patchelf --set-rpath em massa em libQt6*/libKF6*: o patchelf
# desloca secções ELF e deixa DT_INIT órfão (SIGSEGV em dl_init no
# libQt6Svg.so.6 e afins). RUNPATH só no binário e no updater.
if command -v patchelf >/dev/null 2>&1; then
  echo "==> Ajustando RUNPATH só do binário/updater (sem patchelf em massa no Qt)..."
  if [[ -x "$APPDIR/usr/bin/${BINARY}" ]]; then
    patchelf --set-rpath '$ORIGIN/../lib' "$APPDIR/usr/bin/${BINARY}" || true
  fi
  if [[ -x "$APPDIR/usr/bin/appimageupdatetool" ]]; then
    patchelf --set-rpath '$ORIGIN/../lib/appimageupdate:$ORIGIN/../lib' \
      "$APPDIR/usr/bin/appimageupdatetool" || true
    if [[ -d "$APPDIR/usr/lib/appimageupdate" ]]; then
      find "$APPDIR/usr/lib/appimageupdate" -name '*.so*' -type f \
        -exec patchelf --set-rpath '$ORIGIN' {} \; 2>/dev/null || true
    fi
  fi
  # libqxcb: sem RPATH agressivo — usa libxcb do host.
  if [[ -f "$APPDIR/usr/plugins/platforms/libqxcb.so" ]]; then
    patchelf --remove-rpath "$APPDIR/usr/plugins/platforms/libqxcb.so" 2>/dev/null || true
  fi
else
  echo "Aviso: patchelf ausente — RPATHs do binário podem ficar inconsistentes." >&2
fi

# linuxdeploy/patchelf desloca secções ELF e deixa DT_INIT órfão
# (SIGSEGV em dl_init: QtSvg, QuickControls2Fusion, …).
# Substitui no AppDir as libQt6* já empacotadas por cópias limpas do host.
echo "==> Restaurando libQt6* empacotadas a partir do host (ELF intacto)..."
qt6_host_lib() {
  if [[ -d /usr/lib64 ]]; then
    echo /usr/lib64
  else
    echo /usr/lib
  fi
}

restore_one_qt6_soname() {
  local soname="$1"
  local host
  host="$(qt6_host_lib)"
  local host_link="$host/$soname"
  [[ -e "$host_link" ]] || return 1
  local real
  real="$(readlink -f "$host_link")"
  [[ -f "$real" ]] || return 1
  local real_base
  real_base="$(basename "$real")"
  # Remove ficheiro corrompido que usava o nome do soname.
  rm -f "$APPDIR/usr/lib/$soname"
  cp -aL "$real" "$APPDIR/usr/lib/$real_base"
  ln -sfn "$real_base" "$APPDIR/usr/lib/$soname"
}

# 1) Todas as libQt6*.so.* já no AppDir (ficheiros reais versionados).
host="$(qt6_host_lib)"
for bundled in "$APPDIR/usr/lib"/libQt6*.so.*; do
  [[ -e "$bundled" ]] || continue
  base="$(basename "$bundled")"
  if [[ -L "$bundled" ]]; then
    # soname symlink — recria a partir do host
    if [[ -e "$host/$base" ]]; then
      restore_one_qt6_soname "$base" || true
    fi
    continue
  fi
  # Ficheiro versionado ou .so.6 sem symlink
  if [[ -f "$host/$base" && ! -L "$host/$base" ]]; then
    cp -aL "$host/$base" "$bundled"
  elif [[ "$base" =~ \.so\.[0-9]+$ ]] && [[ -e "$host/$base" ]]; then
    restore_one_qt6_soname "$base" || true
  fi
done

# 2) Lista crítica do arranque (Fusion/Svg) — sempre restaurar.
for need in \
  libQt6Core.so.6 \
  libQt6Gui.so.6 \
  libQt6DBus.so.6 \
  libQt6Network.so.6 \
  libQt6OpenGL.so.6 \
  libQt6Qml.so.6 \
  libQt6Quick.so.6 \
  libQt6QuickControls2.so.6 \
  libQt6QuickControls2Fusion.so.6 \
  libQt6QuickTemplates2.so.6 \
  libQt6Svg.so.6 \
  libQt6Widgets.so.6 \
  libQt6WaylandClient.so.6 \
  libQt6XcbQpa.so.6
do
  restore_one_qt6_soname "$need" || true
done

if [[ ! -e "$APPDIR/usr/lib/libQt6Core.so.6" ]]; then
  echo "Erro: falha ao restaurar libQt6Core.so.6 do host." >&2
  exit 1
fi
if [[ ! -e "$APPDIR/usr/lib/libQt6QuickControls2Fusion.so.6" ]]; then
  echo "Erro: libQt6QuickControls2Fusion.so.6 ausente após restore (estilo Fusion)." >&2
  exit 1
fi

# Smoke: libs críticas do arranque (Fusion + Svg) têm de dlopen sem SEGV.
if command -v python3 >/dev/null 2>&1; then
  echo "==> Smoke dlopen Qt6 (Core/Gui/Qml/Quick/Controls2/Fusion/Svg)..."
  if ! LD_LIBRARY_PATH="$APPDIR/usr/lib" python3 - <<'PY'
import ctypes, os, sys

root = os.environ["LD_LIBRARY_PATH"]
libs = [
    "libQt6Core.so.6",
    "libQt6Gui.so.6",
    "libQt6Network.so.6",
    "libQt6Qml.so.6",
    "libQt6OpenGL.so.6",
    "libQt6Quick.so.6",
    "libQt6QuickControls2.so.6",
    "libQt6QuickControls2Fusion.so.6",
    "libQt6QuickTemplates2.so.6",
    "libQt6Svg.so.6",
]
for name in libs:
    path = os.path.join(root, name)
    if not os.path.exists(path):
        # OpenGL/Templates podem variar; Fusion/Svg/Core são obrigatórios.
        if name in (
            "libQt6Core.so.6",
            "libQt6Gui.so.6",
            "libQt6Qml.so.6",
            "libQt6Quick.so.6",
            "libQt6QuickControls2.so.6",
            "libQt6QuickControls2Fusion.so.6",
            "libQt6Svg.so.6",
        ):
            print(f"MISSING {name}", file=sys.stderr)
            sys.exit(1)
        print(f"skip optional {name}")
        continue
    print(f"load {name}")
    ctypes.CDLL(path, mode=ctypes.RTLD_GLOBAL)
print("Qt6 dlopen OK")
PY
  then
    echo "Erro: dlopen Qt6 falhou (SEGV/ELF) — AppImage quebraria no start." >&2
    exit 1
  fi
fi

if [[ ! -x "$APPDIR/usr/bin/appimageupdatetool" ]]; then
  echo "Erro: appimageupdatetool ausente no AppDir." >&2
  exit 1
fi
# Smoke: tool deve iniciar (exit 127 = lib/ELF quebrado).
set +e
"$APPDIR/usr/bin/appimageupdatetool" -h >/dev/null 2>&1 \
  || "$APPDIR/usr/bin/appimageupdatetool" --help >/dev/null 2>&1 \
  || "$APPDIR/usr/bin/appimageupdatetool" -V >/dev/null 2>&1
TOOL_RC=$?
set -e
if [[ "$TOOL_RC" -eq 127 ]]; then
  echo "Erro: appimageupdatetool não inicia (exit 127) — libs faltando?" >&2
  exit 1
fi
echo "==> appimageupdatetool OK (smoke exit=$TOOL_RC)"

# Catálogo i18n (en_GB) — KI18n ignora po/en (trata como idioma-fonte).
if [[ ! -f "$APPDIR/usr/share/locale/en_GB/LC_MESSAGES/webappstation.mo" ]]; then
  echo "==> Copiando locale/en_GB do build para o AppDir..."
  mkdir -p "$APPDIR/usr/share/locale"
  if [[ -d "$ROOT/build/locale/en_GB" ]]; then
    cp -a "$ROOT/build/locale/en_GB" "$APPDIR/usr/share/locale/"
  elif [[ -d "$ROOT/build-agent/locale/en_GB" ]]; then
    cp -a "$ROOT/build-agent/locale/en_GB" "$APPDIR/usr/share/locale/"
  else
    echo "Erro: webappstation.mo (en_GB) não encontrado para o AppImage." >&2
    exit 1
  fi
fi
if [[ ! -f "$APPDIR/usr/share/locale/en_GB/LC_MESSAGES/webappstation.mo" ]]; then
  echo "Erro: falha ao empacotar traduções em inglês (en_GB)." >&2
  exit 1
fi

# linuxdeploy deixa AppRun -> usr/bin/webappstation; escrever sem rm
# seguiria o symlink e sobrescreveria o binário com este script.
rm -f "$APPDIR/AppRun"
cat > "$APPDIR/AppRun" << 'EOF'
#!/bin/bash
HERE="$(dirname "$(readlink -f "$0")")"
export APPDIR="${APPDIR:-$HERE}"
export PATH="${HERE}/usr/bin:${PATH}"
# Somente libs do AppImage — nunca misturar com LD_LIBRARY_PATH do host.
export LD_LIBRARY_PATH="${HERE}/usr/lib"
export QT_PLUGIN_PATH="${HERE}/usr/plugins"
export QML2_IMPORT_PATH="${HERE}/usr/qml:${HERE}/usr/lib/qml"
export QML_IMPORT_PATH="$QML2_IMPORT_PATH"
export XDG_DATA_DIRS="${HERE}/usr/share${XDG_DATA_DIRS:+:$XDG_DATA_DIRS}"
export WEBAPPSTATION_LOCALE_DIR="${HERE}/usr/share/locale"
# Estilo único portátil (sem org.kde.desktop / Plasma).
export QT_QUICK_CONTROLS_STYLE="${QT_QUICK_CONTROLS_STYLE:-Fusion}"
unset QT_ROOT_PATH
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
