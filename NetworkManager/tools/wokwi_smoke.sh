#!/bin/sh

set -u

fail() {
    printf '[FAIL] %s\n' "$1" >&2
    exit 1
}

pass() {
    printf '[PASS] %s\n' "$1"
}

script_dir=$(CDPATH= cd "$(dirname "$0")" && pwd -P) || fail "cannot resolve script directory"
project_dir=$(CDPATH= cd "$script_dir/.." && pwd -P) || fail "cannot resolve project directory"
wokwi_cli=$script_dir/wokwi_cli.sh
[ -x "$wokwi_cli" ] || fail "missing executable Wokwi wrapper: $wokwi_cli"

command -v python3 >/dev/null 2>&1 || fail "python3 is not installed or not on PATH"
python3 -c 'import sys; raise SystemExit(sys.version_info < (3, 11))' \
    >/dev/null 2>&1 || fail "Python 3.11 or newer is required"

wokwi_config=$project_dir/wokwi.toml
[ -f "$wokwi_config" ] || fail "missing Wokwi configuration: $wokwi_config"

if ! WOKWI_CONFIG="$wokwi_config" python3 - <<'PY'
import os
import sys
import tomllib
from pathlib import Path

config_path = Path(os.environ["WOKWI_CONFIG"])
try:
    with config_path.open("rb") as config_file:
        config = tomllib.load(config_file)
except (OSError, tomllib.TOMLDecodeError) as error:
    print(f"[FAIL] cannot parse {config_path}: {error}", file=sys.stderr)
    raise SystemExit(1)

wokwi = config.get("wokwi")
if not isinstance(wokwi, dict):
    print(f"[FAIL] {config_path} has no [wokwi] table", file=sys.stderr)
    raise SystemExit(1)

firmware_name_file = config_path.parent / "tools" / "firmware_name.txt"
try:
    firmware_name = firmware_name_file.read_text().strip()
except OSError as error:
    print(f"[FAIL] cannot read {firmware_name_file}: {error}", file=sys.stderr)
    raise SystemExit(1)
if not firmware_name:
    print(f"[FAIL] {firmware_name_file} is empty", file=sys.stderr)
    raise SystemExit(1)

for name, suffix in (("elf", ".elf"), ("firmware", ".bin")):
    configured_path = wokwi.get(name)
    if not isinstance(configured_path, str) or not configured_path:
        print(f"[FAIL] {config_path} does not configure a {name} file", file=sys.stderr)
        raise SystemExit(1)
    artifact = Path(configured_path)
    if not artifact.is_absolute():
        artifact = config_path.parent / artifact
    if not artifact.is_file() or artifact.stat().st_size == 0:
        print(f"[FAIL] configured {name} artifact is missing or empty: {artifact}", file=sys.stderr)
        raise SystemExit(1)
    expected_dir = (config_path.parent / ".pio/build/wokwi").resolve()
    if expected_dir not in artifact.resolve().parents:
        print(f"[FAIL] configured {name} is not a wokwi artifact: {artifact}", file=sys.stderr)
        raise SystemExit(1)
    if artifact.name != firmware_name + suffix:
        print(
            f"[FAIL] {name} does not match {firmware_name_file}: "
            f"expected {firmware_name + suffix}, got {artifact.name}",
            file=sys.stderr,
        )
        raise SystemExit(1)
PY
then
    exit 1
fi

serial_log=$(mktemp "${TMPDIR:-/tmp}/wokwi-smoke.XXXXXX") || fail "cannot create serial log"
cli_log=$(mktemp "${TMPDIR:-/tmp}/wokwi-cli.XXXXXX") || fail "cannot create Wokwi log"
wokwi_pid=

cleanup() {
    result=$?
    if [ -n "$wokwi_pid" ]; then
        kill -TERM "$wokwi_pid" 2>/dev/null || :
        attempts=0
        while kill -0 "$wokwi_pid" 2>/dev/null && [ "$attempts" -lt 5 ]; do
            sleep 1
            attempts=$((attempts + 1))
        done
        kill -KILL "$wokwi_pid" 2>/dev/null || :
        wait "$wokwi_pid" 2>/dev/null || :
    fi
    if [ "$result" -ne 0 ]; then
        printf '[INFO] Wokwi CLI log: %s\n' "$cli_log" >&2
        printf '[INFO] Wokwi serial log: %s\n' "$serial_log" >&2
        cat "$cli_log" >&2 2>/dev/null || :
        cat "$serial_log" >&2 2>/dev/null || :
    else
        rm -f "$cli_log" "$serial_log"
    fi
    exit "$result"
}

trap cleanup EXIT
trap 'exit 130' INT
trap 'exit 143' TERM

printf '[INFO] launching Wokwi CLI serial smoke\n'
"$wokwi_cli" --timeout 15000 --serial-log-file "$serial_log" \
    "$project_dir" >"$cli_log" 2>&1 &
wokwi_pid=$!

deadline=$(($(date +%s) + 30))
while [ "$(date +%s)" -lt "$deadline" ]; do
    if ! kill -0 "$wokwi_pid" 2>/dev/null; then
        fail "Wokwi CLI exited before reporting readiness"
    fi
    if grep -Fq '[WOKWI] ready:' "$serial_log" 2>/dev/null; then
        pass "Wokwi firmware reported readiness"
        exit 0
    fi
    sleep 1
done

fail "Wokwi firmware did not report readiness within 30s"
