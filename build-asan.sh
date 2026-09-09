#!/usr/bin/env bash

set -euo pipefail

cd -- "$(dirname -- "${BASH_SOURCE[0]}")"

export OPENSWD3_SANITIZER=address
exec "${OPENSWD3_PYTHON:-python3}" build.py core "$@"
