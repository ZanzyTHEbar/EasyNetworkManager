#!/bin/sh

set -eu

fail() {
    printf '[FAIL] %s\n' "$1" >&2
    exit 1
}

script_dir=$(CDPATH= cd "$(dirname "$0")" && pwd -P) || fail "cannot resolve script directory"
project_dir=$(CDPATH= cd "$script_dir/.." && pwd -P) || fail "cannot resolve project directory"

command -v wokwi-cli >/dev/null 2>&1 || fail "wokwi-cli is not installed or not on PATH"
command -v python3 >/dev/null 2>&1 || fail "python3 is not installed or not on PATH"
python3 -c 'import sys; raise SystemExit(sys.version_info < (3, 11))' \
    >/dev/null 2>&1 || fail "Python 3.11 or newer is required"

env_file=$project_dir/.env.local
if [ -z "${WOKWI_CLI_TOKEN:-}" ] && [ -e "$env_file" ]; then
    [ -f "$env_file" ] || fail "$env_file is not a regular file"

    wokwi_token=$(ENV_LOCAL="$env_file" python3 - <<'PY'
import os
import re
import shlex
import stat
import sys
from pathlib import Path

path = Path(os.environ["ENV_LOCAL"])
file_stat = path.stat()
if path.is_symlink():
    print(f"{path} must not be a symlink", file=sys.stderr)
    raise SystemExit(1)
if stat.S_IMODE(file_stat.st_mode) & 0o077:
    print(f"{path} must not be group/world readable or writable", file=sys.stderr)
    raise SystemExit(1)
if not os.access(path, os.R_OK):
    print(f"{path} is not readable by the current user", file=sys.stderr)
    raise SystemExit(1)

token = None
for line_number, raw_line in enumerate(path.read_text().splitlines(), 1):
    line = raw_line.strip()
    if not line or line.startswith("#"):
        continue
    if line.startswith("export "):
        line = line[7:].lstrip()
    match = re.fullmatch(r"WOKWI_CLI_TOKEN\s*=\s*(.*)", line)
    if match is None:
        print(
            f"{path}:{line_number} contains an unsupported assignment",
            file=sys.stderr,
        )
        raise SystemExit(1)
    if token is not None:
        print(f"{path}:{line_number} defines WOKWI_CLI_TOKEN more than once", file=sys.stderr)
        raise SystemExit(1)
    value = match.group(1).strip()
    try:
        parsed = shlex.split(f"WOKWI_CLI_TOKEN={value}", comments=False, posix=True)
    except ValueError as error:
        print(f"{path}:{line_number} has invalid quoting: {error}", file=sys.stderr)
        raise SystemExit(1)
    if len(parsed) != 1 or not parsed[0].startswith("WOKWI_CLI_TOKEN="):
        print(f"{path}:{line_number} has an invalid token value", file=sys.stderr)
        raise SystemExit(1)
    token = parsed[0].split("=", 1)[1]

if not token:
    print(f"{path} does not define a non-empty WOKWI_CLI_TOKEN", file=sys.stderr)
    raise SystemExit(1)
print(token, end="")
PY
    ) || fail "could not securely load $env_file"
    export WOKWI_CLI_TOKEN="$wokwi_token"
fi

exec wokwi-cli "$@"
