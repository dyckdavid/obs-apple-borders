#!/usr/bin/env bash
# Verify stable OBS IDs and setting keys remain present after refactors.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
FAIL=0

require_match() {
  local label="$1"
  local pattern="$2"
  local file="$3"
  if ! grep -Eq "$pattern" "$file"; then
    echo "FAIL: missing $label in $file"
    FAIL=1
  else
    echo "OK: $label"
  fi
}

echo "== Stable plugin IDs =="
require_match "apple_border_filter" '\.id = "apple_border_filter"' "$ROOT/src/filters/apple-border-filter.c"
require_match "apple_pip_filter" '\.id = "apple_pip_filter"' "$ROOT/src/filters/apple-pip-filter.c"
require_match "apple_frost_filter" '\.id = "apple_frost_filter"' "$ROOT/src/filters/apple-frost-filter.c"
require_match "apple_vignette_filter" '\.id = "apple_vignette_filter"' "$ROOT/src/filters/apple-vignette-filter.c"
require_match "apple_spotlight_filter" '\.id = "apple_spotlight_filter"' "$ROOT/src/filters/apple-spotlight-filter.c"
require_match "apple_backdrop_source" '\.id = "apple_backdrop_source"' "$ROOT/src/sources/apple-backdrop-source.c"
require_match "apple_lower_third_source" '\.id = "apple_lower_third_source"' "$ROOT/src/sources/apple-lower-third-source.c"
require_match "apple_soft_gradient_source" '\.id = "apple_soft_gradient_source"' "$ROOT/src/sources/apple-soft-gradient-source.c"

echo "== Setting keys (sample of public keys) =="
require_match "pip_source" 'SETTING_PIP_SOURCE "pip_source"' "$ROOT/src/filters/apple-pip-filter.c"
require_match "corner_radius default" '"corner_radius"' "$ROOT/src/common/apple-chrome.c"
require_match "blur_amount" '"blur_amount"' "$ROOT/src/filters/apple-frost-filter.c"
require_match "preset" '"preset"' "$ROOT/src/sources/apple-backdrop-source.c"
require_match "style_preset" '"style_preset"' "$ROOT/src/sources/apple-lower-third-source.c"
require_match "palette" '"palette"' "$ROOT/src/sources/apple-soft-gradient-source.c"

echo "== Version =="
require_match "version 1.3.0" '"version": "1.3.0"' "$ROOT/buildspec.json"

if [[ "$FAIL" -ne 0 ]]; then
  echo "Smoke test FAILED"
  exit 1
fi

echo "Smoke test PASSED"
