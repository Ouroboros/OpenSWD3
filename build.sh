#!/usr/bin/env bash

set -euo pipefail

cd -- "$(dirname -- "${BASH_SOURCE[0]}")"

exec "${OPENSWD3_PYTHON:-python3}" build.py "$@"
