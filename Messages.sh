#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
# Extrai strings traduzíveis (fonte em pt_BR) para o catálogo KI18n.
# Uso típico (com scripts do l10n.kde.org no PATH):
#   $EXTRACTRC ... ; $XGETTEXT ...
#
# Sem o toolkit KDE completo, mantenha po/en/webappstation.po sincronizado
# manualmente ao adicionar i18n("...") novos.

# shellcheck disable=SC2086
$XGETTEXT `find src -name '*.cpp' -o -name '*.h' -o -name '*.qml'` -o "$podir"/webappstation.pot 2>/dev/null || true
