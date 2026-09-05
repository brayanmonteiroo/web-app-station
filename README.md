# Web App Station

Transforme sites em aplicativos de desktop no Linux.

O Web App Station cria atalhos no menu de aplicativos que abrem um site em janela própria, com ícone, categoria e (opcionalmente) perfil isolado do navegador — como se fosse um app nativo.

## Funcionalidades

- Listar, criar, editar, remover e lançar Web Apps
- Escolher navegador instalado (Brave, Chrome, Chromium, Firefox, Edge, Vivaldi, Flatpak/Snap, Epiphany, Falkon e outros)
- Perfil isolado (Chromium) ou perfil dedicado (Firefox)
- Barra de navegação (Firefox), modo privado/incógnito
- Parâmetros extras do navegador (ex.: `--start-maximized`)
- Ícone do tema ou busca de favicon online
- Categorias do menu (Web, Internet, Jogos, Escritório, etc.)
- Distribuição via **AppImage**, com verificação de atualizações pela própria aplicação

Os Web Apps ficam em `~/.local/share/applications/` como arquivos `.desktop`. Ícones e perfis ficam em `~/.local/share/web-app-station/`.

## Requisitos

- Linux (testado com Fedora / KDE Plasma)
- CMake 3.22+, Ninja, compilador C++20
- Qt 6 e KDE Frameworks 6 (Kirigami, I18n, CoreAddons, Config, IconThemes)

## Build no Fedora

```bash
sudo dnf install cmake ninja-build gcc-c++ extra-cmake-modules \
  qt6-qtbase-devel qt6-qtdeclarative-devel \
  kf6-kirigami-devel kf6-ki18n-devel kf6-kcoreaddons-devel \
  kf6-kconfig-devel kf6-kiconthemes-devel kf6-qqc2-desktop-style \
  kf6-kirigami kf6-breeze-icons zsync

git clone https://github.com/brayanmonteiroo/web-app-station.git
cd web-app-station

cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/bin/webappstation
```

## Build com Docker

Útil se você não quiser instalar as libs de desenvolvimento no host:

```bash
git clone https://github.com/brayanmonteiroo/web-app-station.git
cd web-app-station

docker build -t webappstation-build .
docker run --rm -v "$PWD":/src:z -w /src webappstation-build \
  bash -lc 'cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release && cmake --build build'

# Rodar (precisa de display no host):
./build/bin/webappstation
```

## AppImage

Gera o AppImage e o arquivo `.zsync` usados nas releases:

```bash
chmod +x packaging/appimage/build.sh
./packaging/appimage/build.sh
```

Artefatos:

- `WebAppStation-<versão>-x86_64.AppImage`
- `WebAppStation-<versão>-x86_64.AppImage.zsync`

Em tags `v*`, o GitHub Actions publica esses arquivos na release. Dentro do AppImage, o botão de atualização verifica e aplica a nova versão e pede para reiniciar.

## Licença

MIT — veja [LICENSE](LICENSE).
