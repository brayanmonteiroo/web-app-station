// SPDX-License-Identifier: MIT
.pragma library

/**
 * Alterna uma flag de parâmetro do navegador na string de parâmetros extras.
 * @param {string} text — valor atual do campo
 * @param {string} flag — ex.: "--start-maximized"
 * @returns {string}
 */
function toggle(text, flag) {
    const parts = String(text || "").trim().split(/\s+/).filter(Boolean)
    const idx = parts.indexOf(flag)
    if (idx >= 0) {
        parts.splice(idx, 1)
    } else {
        parts.push(flag)
    }
    return parts.join(" ")
}

/**
 * @param {string} text
 * @param {string} flag
 * @returns {boolean}
 */
function hasFlag(text, flag) {
    const parts = String(text || "").trim().split(/\s+/).filter(Boolean)
    return parts.indexOf(flag) >= 0
}
