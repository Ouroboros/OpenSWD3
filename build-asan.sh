#!/usr/bin/env bash

set -euo pipefail

cd -- "$(dirname -- "${BASH_SOURCE[0]}")"

export OPENSWD3_SANITIZER=address
exec ./build.sh core
