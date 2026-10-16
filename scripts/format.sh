#!/usr/bin/env bash
# Formats the given C++ files (default: all of src/ and tests/) with clang-format and the blank-line rule.
set -eu
cd "$(dirname "$0")/.."
if [ "$#" -eq 0 ]; then
    set -- $(find src tests -name '*.h' -o -name '*.cpp')
fi
/opt/llvm/bin/clang-format -i --style=file:.clang-format "$@"
python3 scripts/blank_lines.py "$@"
