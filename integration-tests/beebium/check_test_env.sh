#!/usr/bin/env bash
# Preflight: required repo roots + pytest collect smoke (optional).
set -euo pipefail

here="$(cd "$(dirname "$0")" && pwd)"

require_var() {
  if [[ -z "${!1:-}" ]]; then
    echo "ERROR: $1 must be set (see docs/DEVELOPMENT.md)" >&2
    exit 1
  fi
}

for var in BEEBIUM_HOME FUJINET_NIO_HOME FN_ROM_HOME; do
  require_var "$var"
  [[ -d "${!var}" ]] || { echo "ERROR: $var not a directory: ${!var}" >&2; exit 1; }
done

if [[ "${CHECK_TEST_ENV_SMOKE:-1}" == "1" ]]; then
  "${here}/run_pytest.sh" --collect-only -q >/dev/null
fi

echo "==> Beebium test environment OK"
