#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
OUT_DIR="${ROOT_DIR}/build"
AWESOME_DIR="${ROOT_DIR}/awesome"
STYLE_DIR="${ROOT_DIR}/style"

# Pin to a known tag; override via env AWESOME_VERSION=v2.4.0
AWESOME_VERSION="${AWESOME_VERSION:-v2.4.0}"

echo ">>> Detalloc docs: preparing doxygen-awesome-css (${AWESOME_VERSION})…"
mkdir -p "${OUT_DIR}" "${STYLE_DIR}"

# Clone theme if missing (robust; no raw URLs)
if [ ! -f "${AWESOME_DIR}/doxygen-awesome.css" ]; then
  rm -rf "${AWESOME_DIR}.tmp"
  git clone --depth=1 --branch "${AWESOME_VERSION}" https://github.com/jothepro/doxygen-awesome-css.git "${AWESOME_DIR}.tmp"
  rm -rf "${AWESOME_DIR}"
  mv "${AWESOME_DIR}.tmp" "${AWESOME_DIR}"
fi

# Minimal brand override
mkdir -p "${STYLE_DIR}"
if [ ! -f "${STYLE_DIR}/detalloc.css" ]; then
  cat > "${STYLE_DIR}/detalloc.css" <<'CSS'
:root { --primary-color:#59d3a5; --primary-dark-color:#39b488; }
code, pre { font-variant-ligatures: none; }
CSS
fi

# Flip HAVE_DOT if graphviz is missing
DOXY="${ROOT_DIR}/Doxyfile"
if ! command -v dot >/dev/null 2>&1; then
  echo ">>> 'dot' (graphviz) not found — disabling DOT features for this run."
  tmp="$(mktemp)"
  sed -E 's/^HAVE_DOT[[:space:]]*=.*/HAVE_DOT = NO/' "${DOXY}" > "${tmp}"
  PROJECT_NUMBER="${PROJECT_NUMBER:-0.1.0}" doxygen "${tmp}"
  rm -f "${tmp}"
else
  echo ">>> Running Doxygen…"
  PROJECT_NUMBER="${PROJECT_NUMBER:-0.1.0}" doxygen "${DOXY}"
fi

echo ">>> Done. Open: ${OUT_DIR}/html/index.html"
