# AGENTS.md — Web App Station

Instruções para agentes de IA neste repositório. Siga à risca.

## Stack

- C++20 / Qt 6 / KF6 (Kirigami, KI18n, KCoreAddons, KConfig, KIconThemes)
- CMake + Ninja; testes com Qt Test / CTest (`autotests/`)
- Empacote: AppImage (`packaging/appimage/build.sh`)
- CI: GitHub Actions em **Fedora 44** (`.github/workflows/tests.yml`, `appimage.yml`)

## Idioma

- Código, comentários novos e UI-fonte: **português (pt_BR)** como msgid KI18n
- Tradução inglesa em `po/en_GB/` (**não** `po/en`: KI18n trata `en`/`en_US` como fonte e ignora o `.mo`)

- Mensagens de commit, PR e release notes: **português**
- Conventional Commits: `tipo(escopo): resumo no imperativo`

## Git (obrigatório)

- **Nunca** incluir `Co-authored-by: Cursor <cursoragent@cursor.com>` (nem variante)
- Se o Cursor injetar co-autor no `git commit`, reescrever com `git commit-tree` + `git reset --hard <hash>`
- Só commit/push/tag quando o usuário pedir
- Push/tag via SSH com o config do usuário (sem HTTPS/`gh auth login` pedindo senha):

```bash
export GIT_SSH_COMMAND='ssh -F /home/brayan/.ssh/config -o BatchMode=yes'
git remote set-url origin git@github.com:brayanmonteiroo/web-app-station.git
git push origin HEAD
```

- Host GitHub: `github.com` → IdentityFile `~/.ssh/github-brayan`
- Não usar `ssh -F /dev/null` (perde a chave). Evitar askpass/ksshaskpass.

## Release AppImage

1. Rodar testes locais: `ctest --test-dir <build> --output-on-failure` (e/ou Docker Fedora 44)
2. Bump `project(webappstation VERSION x.y.z)` em `CMakeLists.txt`
3. Commit limpo (sem co-autor Cursor)
4. `git push origin HEAD` e `git tag -a vX.Y.Z -m "vX.Y.Z" && git push origin vX.Y.Z`
5. Workflow `AppImage` sobe assets na Release; confirmar URL + `.AppImage` / `.zsync`

## i18n (KI18n)

- Msgids da UI são **português (pt_BR)**
- Inglês mora em `po/en_GB/` — **nunca** `po/en` ou `setLanguages({"en"})`: KI18n trata `en`/`en_US` como fonte e devolve o msgid sem ler o `.mo`
- Preferência do usuário pode continuar `language=en` no config; o serviço mapeia para `en_GB`

- Reproduzir falhas **no mesmo container** do workflow (`fedora:44`), com `env -i LANG=C.UTF-8` se quiser simular CI
- Instalar `glibc-langpack-en` (locale `en_GB.UTF-8`): sem locale gerado + `LANG=C`, KI18n/gettext devolve o msgid
- AppImage: script falha alto (`set -e`); não engolir erro de `linuxdeploy` / updater / locale `.mo`
- Logs de Actions privados exigem auth; na dúvida, reproduzir localmente com Docker

## Código

- `declare(strict` não se aplica a C++; preferir tipos explícitos, RAII, serviços finos (`src/core/`)
- UI QML em `src/qml/`; lógica em C++ (`AppController`, services)
- Preferência de idioma: `LocaleService` mapeia `en` → catálogo `en_GB`, ajusta `LANGUAGE`/`LANG` e chama `setlocale(LC_ALL, "")` (necessário quando o processo nasce com `LANG=C.UTF-8`)
- Updates AppImage: `UpdateService` + `appimageupdatetool` empacotado; não misturar `LD_LIBRARY_PATH` do host no AppRun

## Build local

- Preferir diretório gravável (`build-agent/` se `build/` for root do Docker)
- README: documentar o caminho do binário que o usuário deve rodar

## O que não fazer

- Pedir senha GitHub / trocar remote para HTTPS sem necessidade
- `--force` em main, `--no-verify`, amend de commit já enviado (salvo pedido explícito + regras do usuário)
- Inventar dependências (Pinia, Tailwind, etc. — este repo é Qt/KDE, não Vue)
