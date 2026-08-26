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
base_url=${WOKWI_HTTP_URL:-http://localhost:8180}
user_config=$project_dir/ini/user_config.ini

command -v curl >/dev/null 2>&1 || fail "curl is not installed or not on PATH"
command -v python3 >/dev/null 2>&1 || fail "python3 is not installed or not on PATH"
[ -f "$user_config" ] || fail "missing PlatformIO user configuration: $user_config"

fixture_user=$(USER_CONFIG="$user_config" python3 - <<'PY'
import configparser
import os

config = configparser.RawConfigParser()
config.read(os.environ["USER_CONFIG"])
value = config.get("sim", "api_login", fallback="").strip()
print(value[1:-1] if len(value) >= 2 and value[0] == value[-1] and value[0] in "\"'" else value)
PY
) || fail "cannot read sim.api_login from $user_config"
fixture_password=$(USER_CONFIG="$user_config" python3 - <<'PY'
import configparser
import os

config = configparser.RawConfigParser()
config.read(os.environ["USER_CONFIG"])
value = config.get("sim", "api_password", fallback="").strip()
print(value[1:-1] if len(value) >= 2 and value[0] == value[-1] and value[0] in "\"'" else value)
PY
) || fail "cannot read sim.api_password from $user_config"

[ -n "$fixture_user" ] || fail "sim.api_login is empty in $user_config"
[ -n "$fixture_password" ] || fail "sim.api_password is empty in $user_config"
wrong_credentials=$fixture_user:wrong-password
request_timeout=5
startup_timeout=30

printf '[INFO] waiting for the VS Code/private-gateway endpoint at %s\n' "$base_url"
deadline=$(($(date +%s) + startup_timeout))
while [ "$(date +%s)" -lt "$deadline" ]; do
    if curl --silent --output /dev/null --connect-timeout 1 --max-time 2 \
        "$base_url/"; then
        break
    fi
    sleep 1
done

if [ "$(date +%s)" -ge "$deadline" ]; then
    fail "$base_url did not answer; start Wokwi for VS Code with private gateway forwarding"
fi
pass "HTTP forwarding endpoint is reachable"

assert_status() {
    expected=$1
    label=$2
    shift 2

    actual=$(curl --silent --show-error --connect-timeout 1 \
        --max-time "$request_timeout" "$@" -o /dev/null -w '%{http_code}') ||
        fail "$label: curl request failed"
    if [ "$actual" != "$expected" ]; then
        fail "$label: expected HTTP $expected, got HTTP $actual"
    fi
    pass "$label"
}

assert_status 401 "missing credentials rejected" \
    "$base_url/api/builtin/ping"
assert_status 401 "wrong credentials rejected" \
    --user "$wrong_credentials" "$base_url/api/builtin/ping"
assert_status 200 "fixture credentials accepted for ping" \
    --user "$fixture_user:$fixture_password" "$base_url/api/builtin/ping"
assert_status 200 "fixture credentials accepted for identity" \
    --user "$fixture_user:$fixture_password" "$base_url/update/identity"

config_response=$(curl --silent --show-error --connect-timeout 1 \
    --max-time "$request_timeout" \
    --user "$fixture_user:$fixture_password" \
    "$base_url/api/builtin/getConfig") ||
    fail "authenticated getConfig response fetch failed"
case "$config_response" in
    *"$fixture_password"*)
        fail "getConfig response exposes the fixture password"
        ;;
esac
pass "getConfig response does not expose the fixture password"

printf '[PASS] Wokwi HTTP smoke test\n'
