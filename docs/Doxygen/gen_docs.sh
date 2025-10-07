#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
OUT_DIR="${ROOT_DIR}/build"
AWESOME_DIR="${ROOT_DIR}/awesome"
STYLE_DIR="${ROOT_DIR}/style"

echo ">>> Detalloc docs: preparing doxygen-awesome-css…"
mkdir -p "${OUT_DIR}" "${STYLE_DIR}" "${AWESOME_DIR}"

# Fetch doxygen-awesome-css if missing
# ...
if [ ! -f "${AWESOME_DIR}/doxygen-awesome.css" ]; then
  echo "Downloading doxygen-awesome-css assets…"
  base="https://raw.githubusercontent.com/jothepro/doxygen-awesome-css/refs/heads/main"
  curl -fsSL "${base}/doxygen-awesome.css"                -o "${AWESOME_DIR}/doxygen-awesome.css"
  curl -fsSL "${base}/doxygen-awesome-sidebar-only.css"   -o "${AWESOME_DIR}/doxygen-awesome-sidebar-only.css"
  curl -fsSL "${base}/doxygen-awesome-dark.css"           -o "${AWESOME_DIR}/doxygen-awesome-dark.css"   # <— add this
  curl -fsSL "${base}/doxygen-awesome-darkmode-toggle.js" -o "${AWESOME_DIR}/doxygen-awesome-darkmode-toggle.js"
fi


# Create your override CSS if missing (tiny brand accent)
if [ ! -f "${STYLE_DIR}/detalloc.css" ]; then
  cat > "${STYLE_DIR}/detalloc.css" <<'CSS'
/* Small brand accent on top of doxygen-awesome dark */
:root {
  --primary-color: #59d3a5;         /* teal accent */
  --primary-dark-color: #39b488;
}
.m-a, a { text-decoration-thickness: 1px; }
.m-container h1, .m-container h2, .m-container h3 { letter-spacing: .2px; }
code, pre { font-variant-ligatures: none; }
CSS
fi

# Flip HAVE_DOT off if graphviz missing to avoid noisy errors
DXY="${ROOT_DIR}/Doxyfile"
if ! command -v dot >/dev/null 2>&1; then
  echo ">>> 'dot' (graphviz) not found — disabling DOT features for this run."
  tmpfile="$(mktemp)"
  sed -E 's/^HAVE_DOT[[:space:]]*=.*/HAVE_DOT = NO/' "${DXY}" > "${tmpfile}"
  PROJECT_NUMBER="${PROJECT_NUMBER:-0.1.0}" doxygen "${tmpfile}"
  rm -f "${tmpfile}"
else
  echo ">>> Running Doxygen…"
  PROJECT_NUMBER="${PROJECT_NUMBER:-0.1.0}" doxygen "${DXY}"
fi

echo ">>> Done. Open: ${OUT_DIR}/html/index.html"
